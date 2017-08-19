// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromium_media_lib/audio_sync_reader.h"

#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/shared_memory.h"
#include "base/strings/stringprintf.h"
#include "media/audio/audio_device_thread.h"
#include "media/base/audio_parameters.h"

namespace media {

AudioSyncReader::AudioSyncReader(
    const media::AudioParameters& params,
    std::unique_ptr<base::SharedMemory> shared_memory,
    std::unique_ptr<base::CancelableSyncSocket> socket,
    std::unique_ptr<base::CancelableSyncSocket> foreign_socket)
    : shared_memory_(std::move(shared_memory)),
      socket_(std::move(socket)),
      foreign_socket_(std::move(foreign_socket)),
      packet_size_(shared_memory_->requested_size()),
      renderer_callback_count_(0),
      renderer_missed_callback_count_(0),
      trailing_renderer_missed_callback_count_(0),
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
      maximum_wait_time_(params.GetBufferDuration() / 2),
#else
      // TODO(dalecurtis): Investigate if we can reduce this on all platforms.
      maximum_wait_time_(base::TimeDelta::FromMilliseconds(20)),
#endif
      buffer_index_(0) {
  DCHECK_EQ(static_cast<size_t>(packet_size_),
            sizeof(media::AudioOutputBufferParameters) +
                AudioBus::CalculateMemorySize(params));
  AudioOutputBuffer* buffer =
      reinterpret_cast<AudioOutputBuffer*>(shared_memory_->memory());
  output_bus_ = AudioBus::WrapMemory(params, buffer->audio);
  output_bus_->Zero();
}

AudioSyncReader::~AudioSyncReader() {}

// static
std::unique_ptr<AudioSyncReader> AudioSyncReader::Create(
    const media::AudioParameters& params) {
  base::CheckedNumeric<size_t> memory_size =
      sizeof(media::AudioOutputBufferParameters);
  memory_size += AudioBus::CalculateMemorySize(params);

  std::unique_ptr<base::SharedMemory> shared_memory(new base::SharedMemory());
  std::unique_ptr<base::CancelableSyncSocket> socket(
      new base::CancelableSyncSocket());
  std::unique_ptr<base::CancelableSyncSocket> foreign_socket(
      new base::CancelableSyncSocket());

  if (!memory_size.IsValid() ||
      !shared_memory->CreateAndMapAnonymous(memory_size.ValueOrDie()) ||
      !base::CancelableSyncSocket::CreatePair(socket.get(),
                                              foreign_socket.get())) {
    return nullptr;
  }
  return base::WrapUnique(new AudioSyncReader(params, std::move(shared_memory),
                                              std::move(socket),
                                              std::move(foreign_socket)));
}

std::unique_ptr<base::CancelableSyncSocket>
AudioSyncReader::TakeForeignSocket() {
  DCHECK(foreign_socket_);
  return std::move(foreign_socket_);
}

// media::AudioOutputController::SyncReader implementations.
void AudioSyncReader::RequestMoreData(base::TimeDelta delay,
                                      base::TimeTicks delay_timestamp,
                                      int prior_frames_skipped) {
  // We don't send arguments over the socket since sending more than 4
  // bytes might lead to being descheduled. The reading side will zero
  // them when consumed.
  AudioOutputBuffer* buffer =
      reinterpret_cast<AudioOutputBuffer*>(shared_memory_->memory());
  // Increase the number of skipped frames stored in shared memory.
  buffer->params.frames_skipped += prior_frames_skipped;
  buffer->params.delay = delay.InMicroseconds();
  buffer->params.delay_timestamp =
      (delay_timestamp - base::TimeTicks()).InMicroseconds();

  // Zero out the entire output buffer to avoid stuttering/repeating-buffers
  // in the anomalous case if the renderer is unable to keep up with real-time.
  output_bus_->Zero();

  uint32_t control_signal = 0;
  if (delay.is_max()) {
    // std::numeric_limits<uint32_t>::max() is a special signal which is
    // returned after the browser stops the output device in response to a
    // renderer side request.
    control_signal = std::numeric_limits<uint32_t>::max();
  }

  size_t sent_bytes = socket_->Send(&control_signal, sizeof(control_signal));
  if (sent_bytes != sizeof(control_signal)) {
    const std::string error_message = "ASR: No room in socket buffer.";
    LOG(WARNING) << error_message;
  }
  ++buffer_index_;
}

void AudioSyncReader::Read(AudioBus* dest) {
  ++renderer_callback_count_;
  if (!WaitUntilDataIsReady()) {
    ++trailing_renderer_missed_callback_count_;
    ++renderer_missed_callback_count_;
    if (renderer_missed_callback_count_ <= 100) {
      LOG(WARNING) << "AudioSyncReader::Read timed out, audio glitch count="
                   << renderer_missed_callback_count_;
      if (renderer_missed_callback_count_ == 100)
        LOG(WARNING) << "(log cap reached, suppressing further logs)";
    }
    dest->Zero();
    return;
  }

  trailing_renderer_missed_callback_count_ = 0;

  output_bus_->CopyTo(dest);
}

void AudioSyncReader::Close() {
  socket_->Close();
}

bool AudioSyncReader::WaitUntilDataIsReady() {
  base::TimeDelta timeout = maximum_wait_time_;
  const base::TimeTicks start_time = base::TimeTicks::Now();
  const base::TimeTicks finish_time = start_time + timeout;

  // Check if data is ready and if not, wait a reasonable amount of time for it.
  //
  // Data readiness is achieved via parallel counters, one on the renderer side
  // and one here.  Every time a buffer is requested via UpdatePendingBytes(),
  // |buffer_index_| is incremented.  Subsequently every time the renderer has a
  // buffer ready it increments its counter and sends the counter value over the
  // SyncSocket.  Data is ready when |buffer_index_| matches the counter value
  // received from the renderer.
  //
  // The counter values may temporarily become out of sync if the renderer is
  // unable to deliver audio fast enough.  It's assumed that the renderer will
  // catch up at some point, which means discarding counter values read from the
  // SyncSocket which don't match our current buffer index.
  size_t bytes_received = 0;
  uint32_t renderer_buffer_index = 0;
  while (timeout.InMicroseconds() > 0) {
    bytes_received = socket_->ReceiveWithTimeout(
        &renderer_buffer_index, sizeof(renderer_buffer_index), timeout);
    if (bytes_received != sizeof(renderer_buffer_index)) {
      bytes_received = 0;
      break;
    }

    if (renderer_buffer_index == buffer_index_)
      break;

    // Reduce the timeout value as receives succeed, but aren't the right index.
    timeout = finish_time - base::TimeTicks::Now();
  }

  // Receive timed out or another error occurred.  Receive can timeout if the
  // renderer is unable to deliver audio data within the allotted time.
  if (!bytes_received || renderer_buffer_index != buffer_index_) {
    return false;
  }

  return true;
}

}  // namespace media

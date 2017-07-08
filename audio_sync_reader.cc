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
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
      maximum_wait_time_(params.GetBufferDuration() / 2) {
#else
      // TODO(dalecurtis): Investigate if we can reduce this on all platforms.
      maximum_wait_time_(base::TimeDelta::FromMilliseconds(20)) {
#endif
  DCHECK_EQ(static_cast<size_t>(packet_size_),
            sizeof(media::AudioOutputBufferParameters) +
                AudioBus::CalculateMemorySize(params));
  AudioOutputBuffer* buffer =
      reinterpret_cast<AudioOutputBuffer*>(shared_memory_->memory());
  output_bus_ = AudioBus::WrapMemory(params, buffer->audio);
  output_bus_->Zero();
}

AudioSyncReader::~AudioSyncReader() {
}

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
  assert(false);
}

void AudioSyncReader::Read(AudioBus* dest) {
  assert(false);
}

void AudioSyncReader::Close() {
  socket_->Close();
}

} // namespace media

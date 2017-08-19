// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_MEDIA_LIB_AUDIO_SYNC_READER_H_
#define CHROMIUM_MEDIA_LIB_AUDIO_SYNC_READER_H_

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "base/sync_socket.h"
#include "media/audio/audio_output_controller.h"
#include "media/base/audio_bus.h"

#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#endif

namespace base {
class SharedMemory;
}

namespace media {

class AudioSyncReader : public AudioOutputController::SyncReader {
 public:
  ~AudioSyncReader() override;

  // Returns null on failure.
  static std::unique_ptr<AudioSyncReader> Create(
      const media::AudioParameters& params);

  base::SharedMemory* shared_memory() const { return shared_memory_.get(); }

  std::unique_ptr<base::CancelableSyncSocket> TakeForeignSocket();

  // media::AudioOutputController::SyncReader implementations.
  void RequestMoreData(base::TimeDelta delay,
                       base::TimeTicks delay_timestamp,
                       int prior_frames_skipped) override;
  void Read(media::AudioBus* dest) override;
  void Close() override;

 private:
  AudioSyncReader(const media::AudioParameters& params,
                  std::unique_ptr<base::SharedMemory> shared_memory,
                  std::unique_ptr<base::CancelableSyncSocket> socket,
                  std::unique_ptr<base::CancelableSyncSocket> foreign_socket);
  bool WaitUntilDataIsReady();

  std::unique_ptr<base::SharedMemory> shared_memory_;
  std::unique_ptr<base::CancelableSyncSocket> socket_;
  std::unique_ptr<base::CancelableSyncSocket> foreign_socket_;
  std::unique_ptr<media::AudioBus> output_bus_;
  const int packet_size_;

  size_t renderer_callback_count_;
  size_t renderer_missed_callback_count_;
  size_t trailing_renderer_missed_callback_count_;

  const base::TimeDelta maximum_wait_time_;
  uint32_t buffer_index_;

  DISALLOW_COPY_AND_ASSIGN(AudioSyncReader);
};
}

#endif  // CHROMIUM_MEDIA_LIB_AUDIO_SYNC_READER_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#ifndef CHROMIUM_MEDIA_LIB_AUDIO_OUTPUT_DELEGATE_IMPL_H_
#define CHROMIUM_MEDIA_LIB_AUDIO_OUTPUT_DELEGATE_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "media/base/media_export.h"
#include "media/audio/audio_output_delegate.h"

namespace media {

class AudioLog;
class AudioManager;
class AudioOutputController;
class AudioParameters;
class AudioSyncReader;

class MEDIA_EXPORT AudioOutputDelegateImpl
    : public AudioOutputDelegate {
 public:
  AudioOutputDelegateImpl(EventHandler* handler,
                          AudioManager* audio_manager,
                          std::unique_ptr<AudioLog> audio_log,
                          int stream_id,
                          int render_frame_id,
                          const AudioParameters& params,
                          const std::string& output_device_id,
                          base::SingleThreadTaskRunner* io_task_runner);

  ~AudioOutputDelegateImpl() override;

  // AudioOutputDelegate implementation.
  scoped_refptr<AudioOutputController> GetController() const override;
  int GetStreamId() const override;
  void OnPlayStream() override;
  void OnPauseStream() override;
  void OnSetVolume(double volume) override;

 private:
  class ControllerEventHandler;

  void SendCreatedNotification();
  void OnError();
  void UpdatePlayingState(bool playing);

  EventHandler* subscriber_;
  std::unique_ptr<AudioLog> const audio_log_;
  std::unique_ptr<ControllerEventHandler> controller_event_handler_;
  std::unique_ptr<AudioSyncReader> reader_;
  scoped_refptr<AudioOutputController> controller_;
  const int stream_id_;
  base::SingleThreadTaskRunner* const io_task_runner_;

  bool playing_ = false;
  base::WeakPtrFactory<AudioOutputDelegateImpl> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AudioOutputDelegateImpl);
};

} // namespace media

#endif // CHROMIUM_MEDIA_LIB_AUDIO_OUTPUT_DELEGATE_IMPL_H_

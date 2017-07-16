// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#include "chromium_media_lib/audio_device_factory.h"

#include "base/memory/ptr_util.h"
#include "chromium_media_lib/audio_message_filter.h"
#include "chromium_media_lib/media_context.h"
#include "media/audio/audio_output_device.h"
#include "media/base/audio_renderer_mixer_input.h"

namespace media {

#if defined(OS_WIN) || defined(OS_MACOSX)
// Due to driver deadlock issues on Windows (http://crbug/422522) there is a
// chance device authorization response is never received from the browser side.
// In this case we will time out, to avoid renderer hang forever waiting for
// device authorization (http://crbug/615589). This will result in "no audio".
// There are also cases when authorization takes too long on Mac.
const int64_t kMaxAuthorizationTimeoutMs = 10000;
#else
const int64_t kMaxAuthorizationTimeoutMs = 0;  // No timeout.
#endif
const int64_t kHungRendererDelayMs = 30000;

scoped_refptr<media::AudioRendererSink> AudioDeviceFactory::NewAudioRendererMixerSink(
    int owner_id,
    int session_id,
    const std::string& device_id,
    const url::Origin& security_origin) {
  AudioMessageFilter* const filter = AudioMessageFilter::Get();

  scoped_refptr<AudioOutputDevice> device(new media::AudioOutputDevice(
      filter->CreateAudioOutputIPC(owner_id), filter->io_task_runner(),
      session_id, device_id, security_origin,
      // Set authorization request timeout at 80% of renderer hung timeout, but
      // no more than kMaxAuthorizationTimeout.
      base::TimeDelta::FromMilliseconds(std::min(kHungRendererDelayMs * 8 / 10,
                                                 kMaxAuthorizationTimeoutMs))));
  device->RequestDeviceAuthorization();
  return device;
}

scoped_refptr<media::SwitchableAudioRendererSink>
AudioDeviceFactory::NewSwitchableAudioRendererSink(
    int owner_id,
    int session_id,
    const std::string& device_id,
    const url::Origin& security_origin) {
  return scoped_refptr<media::AudioRendererMixerInput>(
      MediaContext::Get()->GetAudioRendererMixerManager()->CreateInput(
          owner_id, session_id, device_id, security_origin,
          media::AudioLatency::LATENCY_PLAYBACK));
}

} // namespace media

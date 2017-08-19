// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#ifndef CHROMIUM_MEDIA_LIB_AUDIO_DEVICE_FACTORY_H_
#define CHROMIUM_MEDIA_LIB_AUDIO_DEVICE_FACTORY_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/audio_latency.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/media_export.h"
#include "media/base/output_device_info.h"

namespace media {

class MEDIA_EXPORT AudioDeviceFactory {
 public:
  static scoped_refptr<media::AudioRendererSink> NewAudioRendererMixerSink(
      int owner_id,
      int session_id,
      const std::string& device_id,
      const url::Origin& security_origin);
  static scoped_refptr<media::SwitchableAudioRendererSink>
  NewSwitchableAudioRendererSink(int owner_frame_id,
                                 int session_id,
                                 const std::string& device_id,
                                 const url::Origin& security_origin);
};

}  // namespace media

#endif  // CHROMIUM_MEDIA_LIB_AUDIO_DEVICE_FACTORY_H_

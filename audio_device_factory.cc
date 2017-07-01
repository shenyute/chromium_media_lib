#include "chromium_media_lib/audio_device_factory.h"

#include "media/base/audio_renderer_mixer_input.h"
#include "chromium_media_lib/media_context.h"

namespace media {

scoped_refptr<media::AudioRendererSink> AudioDeviceFactory::NewAudioRendererMixerSink(
    int owner_id,
    int session_id,
    const std::string& device_id,
    const url::Origin& security_origin) {
  return scoped_refptr<media::AudioRendererMixerInput>(
      MediaContext::Get()->GetAudioRendererMixerManager()->CreateInput(
          owner_id, session_id, device_id, security_origin,
          media::AudioLatency::LATENCY_PLAYBACK));
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

#include "chromium_media_lib/audio_renderer_host.h"

#include "base/bind.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_system.h"

namespace media {

AudioRendererHost::AudioRendererHost(media::AudioManager* audio_manager,
                    media::AudioSystem* audio_system)
    : audio_manager_(audio_manager),
      audio_system_(audio_system) {
  DCHECK(audio_manager_);
}

AudioRendererHost::~AudioRendererHost() {
}

void AudioRendererHost::RequestDeviceAuthorization(int stream_id,
                                int render_frame_id,
                                int session_id,
                                const std::string& device_id,
                                const url::Origin& security_origin,
                                AuthorizationCompletedCallback cb) {
  if (media::AudioDeviceDescription::IsDefaultDevice(device_id)) {
    // Default device doesn't need authorization.
    audio_system_->GetOutputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(&AudioRendererHost::DeviceParametersReceived,
          base::Unretained(this), stream_id, std::move(cb),
          media::AudioDeviceDescription::kDefaultDeviceId));
    return;
  }
  assert(false);
}

void AudioRendererHost::DeviceParametersReceived(
    int stream_id,
    AuthorizationCompletedCallback cb,
    const std::string& raw_device_id,
    const media::AudioParameters& output_params) const {
  DCHECK(!raw_device_id.empty());

  cb.Run(stream_id, media::OUTPUT_DEVICE_STATUS_OK,
         output_params.IsValid() ? output_params
                                 : media::AudioParameters::UnavailableDeviceParams(),
         raw_device_id);
}

void AudioRendererHost::OnStreamCreated(
    int stream_id,
    base::SharedMemory* shared_memory,
    std::unique_ptr<base::CancelableSyncSocket> foreign_socket) {
}

void AudioRendererHost::OnStreamError(int stream_id) {
}

} // namespace media

#include "chromium_media_lib/audio_renderer_host.h"

#include "base/bind.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_system.h"

namespace media {

void AudioRendererHost::SetClient(AudioRendererHostClient* client) {
  client_ = client;
}

AudioRendererHost::AudioRendererHost(media::AudioManager* audio_manager,
                    media::AudioSystem* audio_system,
                    AudioRendererHostClient* client)
    : audio_manager_(audio_manager),
      audio_system_(audio_system),
      client_(client) {
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
    authorizations_.insert(
        std::make_pair(stream_id, std::make_pair(true,
          media::AudioDeviceDescription::kDefaultDeviceId)));
    audio_system_->GetOutputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(&AudioRendererHost::DeviceParametersReceived,
          base::Unretained(this), stream_id, std::move(cb),
          media::AudioDeviceDescription::kDefaultDeviceId));
    return;
  }
  assert(false);
}

void AudioRendererHost::CreateStream(int stream_id,
      int render_frame_id,
      const media::AudioParameters& params) {
  const auto& auth_data = authorizations_.find(stream_id);
  std::string device_unique_id;
  if (auth_data != authorizations_.end()) {
    if (!auth_data->second.first) {
      OnStreamError(stream_id);
      return;
    }
    device_unique_id.swap(auth_data->second.second);
    authorizations_.erase(auth_data);
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
  if (client_)
    client_->OnStreamError(stream_id);
}

AudioRendererHost::AudioOutputDelegateVector::iterator
AudioRendererHost::LookupIteratorById(int stream_id) {
  return std::find_if(
      delegates_.begin(), delegates_.end(),
      [stream_id](const std::unique_ptr<media::AudioOutputDelegate>& d) {
        return d->GetStreamId() == stream_id;
      });
}

void AudioRendererHost::OnCloseStream(int stream_id) {
  authorizations_.erase(stream_id);

  auto i = LookupIteratorById(stream_id);

  // Prevent oustanding callbacks from attempting to close/delete the same
  // AudioOutputDelegate twice.
  if (i == delegates_.end())
    return;

  std::swap(*i, delegates_.back());
  delegates_.pop_back();
}

} // namespace media

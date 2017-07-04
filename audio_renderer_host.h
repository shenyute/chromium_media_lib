#include "media/base/media_export.h"

#include <string>

#include "base/callback.h"
#include "media/audio/audio_output_delegate.h"
#include "media/base/output_device_info.h"
#include "url/origin.h"

namespace media {

class AudioManager;
class AudioParameters;
class AudioSystem;

class MEDIA_EXPORT AudioRendererHost
    : public AudioOutputDelegate::EventHandler {
 public:
  AudioRendererHost(media::AudioManager* audio_manager,
                    media::AudioSystem* audio_system);

  using AuthorizationCompletedCallback =
      base::Callback<void(
                          int stream_id,
                          media::OutputDeviceStatus status,
                          const media::AudioParameters& output_params,
                          const std::string& matched_device_id)>;

  void RequestDeviceAuthorization(int stream_id,
                                  int render_frame_id,
                                  int session_id,
                                  const std::string& device_id,
                                  const url::Origin& security_origin,
                                  AuthorizationCompletedCallback cb);

  // AudioOutputDelegate::EventHandler implementation
  void OnStreamCreated(
      int stream_id,
      base::SharedMemory* shared_memory,
      std::unique_ptr<base::CancelableSyncSocket> foreign_socket) override;
  void OnStreamError(int stream_id) override;

  ~AudioRendererHost() override;

 private:
  void DeviceParametersReceived(
      int stream_id,
      AuthorizationCompletedCallback cb,
      const std::string& raw_device_id,
      const media::AudioParameters& output_params) const;

  media::AudioManager* const audio_manager_;
  media::AudioSystem* const audio_system_;
};

} // namespace media

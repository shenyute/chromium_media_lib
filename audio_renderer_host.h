#ifndef CHROMIUM_MEDIA_LIB_AUDIO_RENDERER_HOST_H_
#define CHROMIUM_MEDIA_LIB_AUDIO_RENDERER_HOST_H_

#include "media/base/media_export.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/shared_memory_handle.h"
#include "base/sync_socket.h"
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
  class AudioRendererHostClient {
   public:
     virtual void OnStreamCreated(int stream_id,
         base::SharedMemoryHandle handle,
         base::SyncSocket::TransitDescriptor socket_descriptor,
         uint32_t length) = 0;
     virtual void OnStreamError(int stream_id) = 0;
  };
  void SetClient(AudioRendererHostClient* client);

  AudioRendererHost(AudioManager* audio_manager,
                    AudioSystem* audio_system,
                    AudioRendererHostClient* client);

  using AuthorizationCompletedCallback =
      base::Callback<void(
                          int stream_id,
                          OutputDeviceStatus status,
                          const AudioParameters& output_params,
                          const std::string& matched_device_id)>;

  void RequestDeviceAuthorization(int stream_id,
                                  int render_frame_id,
                                  int session_id,
                                  const std::string& device_id,
                                  const url::Origin& security_origin,
                                  AuthorizationCompletedCallback cb);

  void CreateStream(int stream_id,
      int render_frame_id,
      const AudioParameters& params);
  void PlayStream(int stream_id);

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
      const AudioParameters& output_params) const;

  using AudioOutputDelegateVector =
      std::vector<std::unique_ptr<AudioOutputDelegate>>;
  AudioOutputDelegateVector::iterator
      LookupIteratorById(int stream_id);
  void OnCloseStream(int stream_id);
  AudioOutputDelegate* LookupById(int stream_id);

  AudioManager* const audio_manager_;
  AudioSystem* const audio_system_;
  std::map<int, std::pair<bool, std::string>> authorizations_;
  AudioRendererHostClient* client_;
  AudioOutputDelegateVector delegates_;
};

} // namespace media

#endif // CHROMIUM_MEDIA_LIB_AUDIO_RENDERER_HOST_H_

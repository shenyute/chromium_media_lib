#ifndef CHROMIUM_MEDIA_LIB_AUDIOSOURCE_PROVIDER_IMPL_H_
#define CHROMIUM_MEDIA_LIB_AUDIOSOURCE_PROVIDER_IMPL_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "media/base/audio_renderer_sink.h"

namespace media {

class MediaLog;

class MEDIA_EXPORT AudioSourceProviderImpl
    : NON_EXPORTED_BASE(public SwitchableAudioRendererSink) {
 public:
  using CopyAudioCB = base::Callback<void(std::unique_ptr<AudioBus>,
                                          uint32_t frames_delayed,
                                          int sample_rate)>;
  AudioSourceProviderImpl(scoped_refptr<SwitchableAudioRendererSink> sink,
                             scoped_refptr<MediaLog> media_log);

  // RestartableAudioRendererSink implementation.
  void Initialize(const AudioParameters& params,
                          RenderCallback* callback) override;
  void Start() override;
  void Stop() override;
  void Play() override;
  void Pause() override;
  bool SetVolume(double volume) override;
  OutputDeviceInfo GetOutputDeviceInfo() override;
  bool CurrentThreadIsRenderingThread() override;
  void SwitchOutputDevice(const std::string& device_id,
                          const url::Origin& security_origin,
                          const OutputDeviceStatusCB& callback) override;
 protected:
  scoped_refptr<SwitchableAudioRendererSink> CreateFallbackSink();
  ~AudioSourceProviderImpl() override;

 private:

  double volume_;
  base::Lock sink_lock_;
  scoped_refptr<SwitchableAudioRendererSink> sink_;

  class TeeFilter;
  const std::unique_ptr<TeeFilter> tee_filter_;

  scoped_refptr<MediaLog> const media_log_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<AudioSourceProviderImpl> weak_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AudioSourceProviderImpl);
};

}

#endif // CHROMIUM_MEDIA_LIB_AUDIOSOURCE_PROVIDER_IMPL_H_

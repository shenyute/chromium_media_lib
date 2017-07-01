#include <memory>

#include "base/task_scheduler/task_scheduler.h"
#include "chromium_media_lib/audio_renderer_mixer_manager.h"
#include "media/base/decoder_factory.h"

namespace media {

class MEDIA_EXPORT MediaContext {
 public:
  static MediaContext* Get();

  MediaContext();
  ~MediaContext();
  DecoderFactory* GetDecoderFactory();
  AudioRendererMixerManager* GetAudioRendererMixerManager();
  std::unique_ptr<base::TaskScheduler::InitParams>
      GetDefaultTaskSchedulerInitParams();

 private:
  std::unique_ptr<AudioRendererMixerManager> audio_renderer_mixer_manager_;
  std::unique_ptr<DecoderFactory> decoder_factory_;
};

} // namespace media

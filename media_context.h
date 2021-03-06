// Copyright (c) 2017 YuTeh Shen
//

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "chromium_media_lib/audio_message_filter.h"
#include "chromium_media_lib/audio_renderer_host.h"
#include "chromium_media_lib/audio_renderer_mixer_manager.h"
#include "media/audio/audio_manager.h"
#include "media/base/decoder_factory.h"

namespace media {

class AudioSystem;

class MEDIA_EXPORT MediaContext {
 public:
  static MediaContext* Get();

  MediaContext();
  ~MediaContext();
  DecoderFactory* GetDecoderFactory();
  AudioRendererMixerManager* GetAudioRendererMixerManager();
  std::unique_ptr<base::TaskScheduler::InitParams>
  GetDefaultTaskSchedulerInitParams();
  base::SingleThreadTaskRunner* io_task_runner() const;
  AudioManager* audio_manager() const { return audio_manager_.get(); }
  AudioSystem* audio_system() const { return audio_system_.get(); }
  AudioRendererHost* audio_renderer_host() const {
    return audio_renderer_host_.get();
  }

 private:
  std::unique_ptr<AudioRendererMixerManager> audio_renderer_mixer_manager_;
  std::unique_ptr<DecoderFactory> decoder_factory_;
  std::unique_ptr<base::Thread> io_thread_;
  scoped_refptr<AudioMessageFilter> audio_message_filter_;
  std::unique_ptr<media::AudioManager> audio_manager_;
  std::unique_ptr<AudioSystem> audio_system_;
  std::unique_ptr<AudioRendererHost> audio_renderer_host_;
};

}  // namespace media

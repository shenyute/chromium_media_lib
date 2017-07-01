#include "chromium_media_lib/media_context.h"

#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/sys_info.h"
#include "base/threading/thread_local.h"

namespace media {

namespace {

static base::LazyInstance<MediaContext>::DestructorAtExit
    g_context = LAZY_INSTANCE_INITIALIZER;
}

MediaContext* MediaContext::Get() {
  return g_context.Pointer();
}

MediaContext::MediaContext() {
  decoder_factory_.reset(new DecoderFactory());
}

MediaContext::~MediaContext() {
}

DecoderFactory* MediaContext::GetDecoderFactory() {
  return decoder_factory_.get();
}

AudioRendererMixerManager* MediaContext::GetAudioRendererMixerManager() {
  if (!audio_renderer_mixer_manager_) {
    audio_renderer_mixer_manager_ = AudioRendererMixerManager::Create();
  }

  return audio_renderer_mixer_manager_.get();
}

std::unique_ptr<base::TaskScheduler::InitParams>
MediaContext::GetDefaultTaskSchedulerInitParams() {
  using StandbyThreadPolicy =
      base::SchedulerWorkerPoolParams::StandbyThreadPolicy;

  constexpr int kMaxNumThreadsInBackgroundPool = 1;
  constexpr int kMaxNumThreadsInBackgroundBlockingPool = 1;
  constexpr int kMaxNumThreadsInForegroundPoolLowerBound = 2;
  constexpr int kMaxNumThreadsInForegroundBlockingPool = 1;
  constexpr auto kSuggestedReclaimTime = base::TimeDelta::FromSeconds(30);

  return base::MakeUnique<base::TaskScheduler::InitParams>(
      base::SchedulerWorkerPoolParams(StandbyThreadPolicy::LAZY,
                                      kMaxNumThreadsInBackgroundPool,
                                      kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(StandbyThreadPolicy::LAZY,
                                      kMaxNumThreadsInBackgroundBlockingPool,
                                      kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::LAZY,
          std::max(kMaxNumThreadsInForegroundPoolLowerBound,
                   base::SysInfo::NumberOfProcessors()),
          kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(StandbyThreadPolicy::LAZY,
                                      kMaxNumThreadsInForegroundBlockingPool,
                                      kSuggestedReclaimTime));
}

} // namespace media

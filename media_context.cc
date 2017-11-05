// Copyright (c) 2017 YuTeh Shen
//
#include "chromium_media_lib/media_context.h"

#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/sys_info.h"
#include "base/threading/thread_local.h"
#include "chromium_media_lib/media_internals.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/audio_thread_impl.h"

namespace media {

namespace {

static base::LazyInstance<MediaContext>::DestructorAtExit g_context =
    LAZY_INSTANCE_INITIALIZER;
}

MediaContext* MediaContext::Get() {
  return g_context.Pointer();
}

MediaContext::MediaContext() : io_thread_(new base::Thread("Media IO")) {
  io_thread_->Start();
  audio_message_filter_ = new AudioMessageFilter(io_thread_->task_runner());
  decoder_factory_.reset(new DecoderFactory());
  audio_manager_ = AudioManager::Create(
      base::MakeUnique<media::AudioThreadImpl>(),
      MediaInternals::GetInstance());
  CHECK(audio_manager_);

  audio_system_ = media::AudioSystemImpl::Create(audio_manager_.get());
  audio_renderer_host_ = base::MakeUnique<AudioRendererHost>(
      audio_manager_.get(), audio_system_.get(), audio_message_filter_.get());
  CHECK(audio_system_);
}

MediaContext::~MediaContext() {}

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
  // Reference GetDefaultTaskSchedulerInitParams
  // from content/renderer/render_process_impl.cc
  constexpr int kMaxNumThreadsInBackgroundPool = 1;
  constexpr int kMaxNumThreadsInBackgroundBlockingPool = 1;
  constexpr int kMaxNumThreadsInForegroundPoolLowerBound = 2;
  constexpr int kMaxNumThreadsInForegroundBlockingPool = 1;
  constexpr auto kSuggestedReclaimTime = base::TimeDelta::FromSeconds(30);

  return base::MakeUnique<base::TaskScheduler::InitParams>(
      base::SchedulerWorkerPoolParams(kMaxNumThreadsInBackgroundPool,
                                      kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(kMaxNumThreadsInBackgroundBlockingPool,
                                      kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(
          std::max(
              kMaxNumThreadsInForegroundPoolLowerBound,
              base::SysInfo::NumberOfProcessors()),
          kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(kMaxNumThreadsInForegroundBlockingPool,
                                      kSuggestedReclaimTime));
}

base::SingleThreadTaskRunner* MediaContext::io_task_runner() const {
  return io_thread_->task_runner().get();
}

}  // namespace media

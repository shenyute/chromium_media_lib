// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#include "chromium_media_lib/mediaplayer_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "chromium_media_lib/audio_device_factory.h"
#include "chromium_media_lib/media_context.h"
#include "media/base/bind_to_current_loop.h"
#include "media/filters/ffmpeg_demuxer.h"
#include "media/renderers/default_renderer_factory.h"

namespace media {

const double kMinRate = 0.0625;
const double kMaxRate = 16.0;

MediaPlayerImpl::MediaPlayerImpl(MediaPlayerParams& params)
    : main_task_runner_(params.main_task_runner()),
      media_task_runner_(params.media_task_runner()),
      worker_task_runner_(params.worker_task_runner()),
      media_log_(params.take_media_log()),
      owner_id_(1),
      playback_rate_(0.0),
      paused_(true),
      seeking_(false),
      ended_(false),
      volume_(1.0),
      video_renderer_sink_(new VideoRendererSinkImpl(media_task_runner_)),
      pipeline_controller_(
          base::MakeUnique<PipelineImpl>(media_task_runner_, media_log_.get()),
          base::Bind(&MediaPlayerImpl::CreateRenderer,
                     base::Unretained(this)),
          base::Bind(&MediaPlayerImpl::OnPipelineSeeked, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnPipelineSuspended, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnBeforePipelineResume, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnPipelineResumed, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnError, AsWeakPtr())) {
  renderer_factory_ = base::MakeUnique<media::DefaultRendererFactory>(
      media_log_.get(), MediaContext::Get()->GetDecoderFactory(),
      DefaultRendererFactory::GetGpuFactoriesCB());
  audio_source_provider_ =
      new AudioSourceProviderImpl(AudioDeviceFactory::NewSwitchableAudioRendererSink(
          owner_id_, 0, "", url::Origin()), media_log_);
}

MediaPlayerImpl::~MediaPlayerImpl() {
}

void MediaPlayerImpl::Load(GURL url) {
}

void MediaPlayerImpl::Load(const base::FilePath& path) {
  data_source_.reset(new FileDataSource(
        path, main_task_runner_));
  data_source_->Initialize(
      base::Bind(&MediaPlayerImpl::DataSourceInitialized, AsWeakPtr()));
}

void MediaPlayerImpl::Play() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  paused_ = false;
  pipeline_controller_.SetPlaybackRate(playback_rate_);
}

void MediaPlayerImpl::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  paused_ = true;
  pipeline_controller_.SetPlaybackRate(0.0);
  paused_time_ =
      ended_ ? GetPipelineMediaDuration() : pipeline_controller_.GetMediaTime();
}

bool MediaPlayerImpl::SupportsSave() const {
  return false;
}

void MediaPlayerImpl::Seek(double seconds) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DoSeek(base::TimeDelta::FromSecondsD(seconds), true);
}

void MediaPlayerImpl::DoSeek(base::TimeDelta time, bool time_updated) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  ended_ = false;
  seeking_ = true;
  if (paused_)
    paused_time_ = time;
  pipeline_controller_.Seek(time, time_updated);
}

void MediaPlayerImpl::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // TODO(kylep): Remove when support for negatives is added. Also, modify the
  // following checks so rewind uses reasonable values also.
  if (rate < 0.0)
    return;

  // Limit rates to reasonable values by clamping.
  if (rate != 0.0) {
    if (rate < kMinRate)
      rate = kMinRate;
    else if (rate > kMaxRate)
      rate = kMaxRate;
  }

  playback_rate_ = rate;
  if (!paused_)
    pipeline_controller_.SetPlaybackRate(rate);
}

void MediaPlayerImpl::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  volume_ = volume;
  pipeline_controller_.SetVolume(volume_);
}

void MediaPlayerImpl::OnError(PipelineStatus status) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  NOTREACHED();
}

base::TimeDelta MediaPlayerImpl::GetPipelineMediaDuration() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  return pipeline_controller_.GetMediaDuration();
}

void MediaPlayerImpl::OnEnded() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  ended_ = true;
}

void MediaPlayerImpl::OnMetadata(PipelineMetadata metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  pipeline_metadata_ = metadata;
}

void MediaPlayerImpl::OnBufferingStateChange(BufferingState state) {
  if (!pipeline_controller_.IsStable())
    return;
}

void MediaPlayerImpl::OnDurationChange() {
}

void MediaPlayerImpl::OnAddTextTrack(const TextTrackConfig& config,
                    const AddTextTrackDoneCB& done_cb) {
  // TODO:
  NOTREACHED();
}

void MediaPlayerImpl::OnWaitingForDecryptionKey() {
  // TODO:
  NOTREACHED();
}

void MediaPlayerImpl::OnVideoNaturalSizeChange(const gfx::Size& size) {
}

void MediaPlayerImpl::OnVideoOpacityChange(bool opaque) {
}

void MediaPlayerImpl::OnVideoAverageKeyframeDistanceUpdate() {
}


void MediaPlayerImpl::SwitchRenderer(bool is_rendered_remotely) {
}

void MediaPlayerImpl::ActivateViewportIntersectionMonitoring(bool activate) {
}

void MediaPlayerImpl::DataSourceInitialized(bool success) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!success) {
    return;
  }
  StartPipeline();
}

void MediaPlayerImpl::StartPipeline() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(data_source_);

  Demuxer::EncryptedMediaInitDataCB encrypted_media_init_data_cb =
      BindToCurrentLoop(base::Bind(
          &MediaPlayerImpl::OnEncryptedMediaInitData, AsWeakPtr()));

#if !defined(MEDIA_DISABLE_FFMPEG)
  Demuxer::MediaTracksUpdatedCB media_tracks_updated_cb =
    BindToCurrentLoop(base::Bind(
          &MediaPlayerImpl::OnFFmpegMediaTracksUpdated, AsWeakPtr()));

  demuxer_.reset(new FFmpegDemuxer(
        media_task_runner_, data_source_.get(), encrypted_media_init_data_cb,
        media_tracks_updated_cb, media_log_.get()));
  bool is_streaming = false;
  pipeline_controller_.Start(demuxer_.get(), this, is_streaming, true);
#else
  OnError(PipelineStatus::DEMUXER_ERROR_COULD_NOT_OPEN);
  return;
#endif
}

void MediaPlayerImpl::OnPipelineSeeked(bool time_updated) {
  seeking_ = false;
  seek_time_ = base::TimeDelta();
  if (paused_) {
    paused_time_ = pipeline_controller_.GetMediaTime();
  }
}

void MediaPlayerImpl::OnPipelineSuspended() {
}

void MediaPlayerImpl::OnBeforePipelineResume() {
}

void MediaPlayerImpl::OnPipelineResumed() {
}

void MediaPlayerImpl::OnDemuxerOpened() {
}

std::unique_ptr<Renderer> MediaPlayerImpl::CreateRenderer() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  RequestSurfaceCB request_surface_cb;
  return renderer_factory_->CreateRenderer(
      media_task_runner_,
      worker_task_runner_,
      audio_source_provider_.get(),
      video_renderer_sink_.get(),
      request_surface_cb);
}

void MediaPlayerImpl::OnEncryptedMediaInitData(
    EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data) {
  // not support yet
  NOTREACHED();
}

void MediaPlayerImpl::OnFFmpegMediaTracksUpdated(std::unique_ptr<MediaTracks> tracks) {
  DCHECK(demuxer_.get());
}

} // namespace media

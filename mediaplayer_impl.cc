#include "chromium_media_lib/mediaplayer_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "media/filters/ffmpeg_demuxer.h"

namespace media {

MediaPlayerImpl::MediaPlayerImpl(MediaPlayerParams& params)
    : main_task_runner_(params.main_task_runner()),
      media_task_runner_(params.media_task_runner()),
      worker_task_runner_(params.worker_task_runner()),
      media_log_(params.take_media_log()),
      pipeline_controller_(
          base::MakeUnique<PipelineImpl>(media_task_runner_, media_log_.get()),
          base::Bind(&MediaPlayerImpl::CreateRenderer,
                     base::Unretained(this)),
          base::Bind(&MediaPlayerImpl::OnPipelineSeeked, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnPipelineSuspended, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnBeforePipelineResume, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnPipelineResumed, AsWeakPtr()),
          base::Bind(&MediaPlayerImpl::OnError, AsWeakPtr())) {
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
}

void MediaPlayerImpl::Pause() {
}

bool MediaPlayerImpl::SupportsSave() const {
  return false;
}

void MediaPlayerImpl::Seek(double seconds) {
}

void MediaPlayerImpl::SetRate(double rate) {
}

void MediaPlayerImpl::SetVolume(double volume) {
}

void MediaPlayerImpl::OnError(PipelineStatus status) {
}

void MediaPlayerImpl::OnEnded() {
}

void MediaPlayerImpl::OnMetadata(PipelineMetadata metadata) {
}

void MediaPlayerImpl::OnBufferingStateChange(BufferingState state) {
}

void MediaPlayerImpl::OnDurationChange() {
}

void MediaPlayerImpl::OnAddTextTrack(const TextTrackConfig& config,
                    const AddTextTrackDoneCB& done_cb) {
}

void MediaPlayerImpl::OnWaitingForDecryptionKey() {
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

  /*
#if !defined(MEDIA_DISABLE_FFMPEG)
  Demuxer::MediaTracksUpdatedCB media_tracks_updated_cb =
    BindToCurrentLoop(base::Bind(
          &MediaPlayerImpl::OnFFmpegMediaTracksUpdated, AsWeakPtr()));

  demuxer_.reset(new FFmpegDemuxer(
        media_task_runner_, data_source_.get(), encrypted_media_init_data_cb,
        media_tracks_updated_cb, media_log_.get()));
#else
  OnError(PipelineStatus::DEMUXER_ERROR_COULD_NOT_OPEN);
  return;
#endif
  */
}

void MediaPlayerImpl::OnPipelineSeeked(bool time_updated) {
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
  return nullptr;
}

} // namespace media

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

MediaPlayerImpl::MediaPlayerImpl(MediaPlayerParams& params)
    : main_task_runner_(params.main_task_runner()),
      media_task_runner_(params.media_task_runner()),
      worker_task_runner_(params.worker_task_runner()),
      media_log_(params.take_media_log()),
      owner_id_(1),
      video_renderer_sink_(new VideoRendererSinkImpl),
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

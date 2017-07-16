// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#ifndef CHROMIUM_MEDIA_LIB_MEDIAPLAYER_IMPL_H_
#define CHROMIUM_MEDIA_LIB_MEDIAPLAYER_IMPL_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromium_media_lib/audiosourceprovider_impl.h"
#include "chromium_media_lib/file_data_source.h"
#include "chromium_media_lib/mediaplayer_params.h"
#include "chromium_media_lib/video_renderer_sink_impl.h"
#include "media/base/media_observer.h"
#include "media/base/media_tracks.h"
#include "media/base/pipeline_impl.h"
#include "media/base/renderer_factory.h"
#include "media/filters/pipeline_controller.h"
#include "url/gurl.h"

namespace media {

class MEDIA_EXPORT MediaPlayerImpl
    : public NON_EXPORTED_BASE(Pipeline::Client),
      public MediaObserverClient,
      public base::SupportsWeakPtr<MediaPlayerImpl> {
 public:
   MediaPlayerImpl(MediaPlayerParams& params);
   ~MediaPlayerImpl() override;

  // Playback controls.
  void Load(GURL url);
  void Load(const base::FilePath& path);
  void Play();
  void Pause();
  bool SupportsSave() const;
  void Seek(double seconds);
  void SetRate(double rate);
  void SetVolume(double volume);
  base::TimeDelta GetPipelineMediaDuration() const;

 private:
  // Pipeline::Client overrides.
  void OnError(PipelineStatus status) override;
  void OnEnded() override;
  void OnMetadata(PipelineMetadata metadata) override;
  void OnBufferingStateChange(BufferingState state) override;
  void OnDurationChange() override;
  void OnAddTextTrack(const TextTrackConfig& config,
                      const AddTextTrackDoneCB& done_cb) override;
  void OnWaitingForDecryptionKey() override;
  void OnVideoNaturalSizeChange(const gfx::Size& size) override;
  void OnVideoOpacityChange(bool opaque) override;
  void OnVideoAverageKeyframeDistanceUpdate() override;

  // MediaObserverClient implementation.
  void SwitchRenderer(bool is_rendered_remotely) override;
  void ActivateViewportIntersectionMonitoring(bool activate) override;

  void DataSourceInitialized(bool success);
  void StartPipeline();

  void OnPipelineSeeked(bool time_updated);
  void OnPipelineSuspended();
  void OnBeforePipelineResume();
  void OnPipelineResumed();
  void OnDemuxerOpened();

  std::unique_ptr<Renderer> CreateRenderer();
  void DoSeek(base::TimeDelta time, bool time_updated);

  void OnEncryptedMediaInitData(EmeInitDataType init_data_type,
                                const std::vector<uint8_t>& init_data);
  void OnFFmpegMediaTracksUpdated(std::unique_ptr<MediaTracks> tracks);

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  scoped_refptr<base::TaskRunner> worker_task_runner_;
  scoped_refptr<MediaLog> media_log_;
  scoped_refptr<AudioSourceProviderImpl> audio_source_provider_;
  int owner_id_;

  PipelineMetadata pipeline_metadata_;
  double playback_rate_;

  bool paused_;
  base::TimeDelta paused_time_;

  bool seeking_;
  base::TimeDelta seek_time_;

  bool ended_;
  double volume_;

  std::unique_ptr<VideoRendererSinkImpl> video_renderer_sink_;
  // |pipeline_controller_| owns an instance of Pipeline.
  PipelineController pipeline_controller_;
  GURL loaded_url_;

  std::unique_ptr<RendererFactory> renderer_factory_;
  std::unique_ptr<FileDataSource> data_source_;
  std::unique_ptr<Demuxer> demuxer_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerImpl);
};

} // namespace media

#endif // CHROMIUM_MEDIA_LIB_MEDIAPLAYER_IMPL_H_

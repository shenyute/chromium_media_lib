// Copyright (c) 2017 YuTeh Shen
//

#include "chromium_media_lib/mediaplayer_params.h"

namespace media {

MediaPlayerParams::MediaPlayerParams(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::TaskRunner> worker_task_runner,
    std::unique_ptr<MediaLog> media_log)
    : main_task_runner_(main_task_runner),
      media_task_runner_(media_task_runner),
      io_task_runner_(io_task_runner),
      worker_task_runner_(worker_task_runner),
      media_log_(std::move(media_log)),
      video_renderer_sink_client_(nullptr) {}

MediaPlayerParams::~MediaPlayerParams() {}

}  // namespace media

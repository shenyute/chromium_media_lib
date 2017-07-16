// Copyright (c) 2017 YuTeh Shen
//

#ifndef CHROMIUM_MEDIA_LIB_MEDIAPLAYER_PARAMS_H_
#define CHROMIUM_MEDIA_LIB_MEDIAPLAYER_PARAMS_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "media/base/media_log.h"

namespace media {

class MediaPlayerParams {
 public:
  MediaPlayerParams(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
      scoped_refptr<base::TaskRunner> worker_task_runner,
      scoped_refptr<MediaLog> media_log);
  scoped_refptr<MediaLog> take_media_log() { return media_log_; }
  ~MediaPlayerParams();

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner() {
    return main_task_runner_;
  }
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner() {
    return media_task_runner_;
  }
  scoped_refptr<base::TaskRunner> worker_task_runner() {
    return worker_task_runner_;
  }


 private:
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  scoped_refptr<base::TaskRunner> worker_task_runner_;
  scoped_refptr<MediaLog> media_log_;
};

}  // namespace media

#endif // CHROMIUM_MEDIA_LIB_MEDIAPLAYER_PARAMS_H_

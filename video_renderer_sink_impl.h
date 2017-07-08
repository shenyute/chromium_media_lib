#ifndef CHROMIUM_MEDIA_LIB_VIDEO_RENDERER_SINK_IMPL_H_
#define CHROMIUM_MEDIA_LIB_VIDEO_RENDERER_SINK_IMPL_H_

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "media/base/media_export.h"
#include "media/base/video_renderer_sink.h"

namespace media {

class MEDIA_EXPORT VideoRendererSinkImpl
    : public VideoRendererSink {
 public:
  VideoRendererSinkImpl();
  ~VideoRendererSinkImpl() override;

  // VideoRendererSink implementation. These methods must be called from the
  // same thread (typically the media thread).
  void Start(RenderCallback* callback) override;
  void Stop() override;
  void PaintSingleFrame(const scoped_refptr<VideoFrame>& frame,
                        bool repaint_duplicate_frame = false) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoRendererSinkImpl);
};

}

#endif // CHROMIUM_MEDIA_LIB_VIDEO_RENDERER_SINK_IMPL_H_

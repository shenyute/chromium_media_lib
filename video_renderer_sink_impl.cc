#include "chromium_media_lib/video_renderer_sink_impl.h"

namespace media {

VideoRendererSinkImpl::VideoRendererSinkImpl() {
}

VideoRendererSinkImpl::~VideoRendererSinkImpl() {
}

void VideoRendererSinkImpl::Start(RenderCallback* callback) {
  assert(false);
}

void VideoRendererSinkImpl::Stop() {
  assert(false);
}

void VideoRendererSinkImpl::PaintSingleFrame(const scoped_refptr<VideoFrame>& frame,
                    bool repaint_duplicate_frame) {
  assert(false);
}

}

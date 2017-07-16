// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#ifndef CHROMIUM_MEDIA_LIB_VIDEO_RENDERER_SINK_IMPL_H_
#define CHROMIUM_MEDIA_LIB_VIDEO_RENDERER_SINK_IMPL_H_

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "media/base/media_export.h"
#include "media/base/video_renderer_sink.h"

namespace media {

class MEDIA_EXPORT VideoRendererSinkClient {
 public:
  virtual void StartRendering() = 0;
  virtual void StopRendering() = 0;
  virtual void DidReceiveFrame(scoped_refptr<VideoFrame> frame) = 0;
};

class MEDIA_EXPORT VideoRendererSinkImpl
    : public VideoRendererSink {
 public:
  VideoRendererSinkImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& compositor_task_runner);
  ~VideoRendererSinkImpl() override;

  void SetVideoRendererSinkClient(VideoRendererSinkClient* client);

  // VideoRendererSink implementation. These methods must be called from the
  // same thread (typically the media thread).
  void Start(RenderCallback* callback) override;
  void Stop() override;
  void PaintSingleFrame(const scoped_refptr<VideoFrame>& frame,
                        bool repaint_duplicate_frame = false) override;

 private:
  bool ProcessNewFrame(
    const scoped_refptr<VideoFrame>& frame,
    bool repaint_duplicate_frame);
  void BackgroundRender();
  bool CallRender(base::TimeTicks deadline_min,
                  base::TimeTicks deadline_max,
                  bool background_rendering);
  void OnRendererStateUpdate(bool new_state);

  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  std::unique_ptr<base::TickClock> tick_clock_;

  bool background_rendering_enabled_;
  base::Timer background_rendering_timer_;

  VideoRendererSinkClient* client_;
  bool rendering_;
  bool rendered_last_frame_;
  bool is_background_rendering_;
  bool new_background_frame_;
  base::TimeDelta last_interval_;

  scoped_refptr<VideoFrame> current_frame_;

  base::Lock callback_lock_;
  VideoRendererSink::RenderCallback* callback_;

  DISALLOW_COPY_AND_ASSIGN(VideoRendererSinkImpl);
};

}

#endif // CHROMIUM_MEDIA_LIB_VIDEO_RENDERER_SINK_IMPL_H_

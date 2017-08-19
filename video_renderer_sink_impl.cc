// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromium_media_lib/video_renderer_sink_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/time/default_tick_clock.h"
#include "media/base/video_frame.h"

namespace media {

const int kBackgroundRenderingTimeoutMs = 250;

VideoRendererSinkImpl::VideoRendererSinkImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& compositor_task_runner)
    : compositor_task_runner_(compositor_task_runner),
      tick_clock_(new base::DefaultTickClock()),
      background_rendering_enabled_(true),
      background_rendering_timer_(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kBackgroundRenderingTimeoutMs),
          base::Bind(&VideoRendererSinkImpl::BackgroundRender,
                     base::Unretained(this)),
          // Task is not repeating, CallRender() will reset the task as needed.
          false),
      client_(nullptr),
      rendering_(false),
      rendered_last_frame_(false),
      is_background_rendering_(false),
      new_background_frame_(false),
      last_interval_(base::TimeDelta::FromSecondsD(1.0 / 60)),
      callback_(nullptr) {
  background_rendering_timer_.SetTaskRunner(compositor_task_runner_);
}

VideoRendererSinkImpl::~VideoRendererSinkImpl() {}

void VideoRendererSinkImpl::SetVideoRendererSinkClient(
    VideoRendererSinkClient* client) {
  if (!compositor_task_runner_->BelongsToCurrentThread()) {
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&VideoRendererSinkImpl::SetVideoRendererSinkClient,
                   base::Unretained(this), client));
    return;
  }
  client_ = client;
  if (rendering_ && client_)
    client_->StartRendering();
}

void VideoRendererSinkImpl::Start(RenderCallback* callback) {
  // Called from the media thread, so acquire the callback under lock before
  // returning in case a Stop() call comes in before the PostTask is processed.
  base::AutoLock lock(callback_lock_);
  DCHECK(!callback_);
  callback_ = callback;
  compositor_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VideoRendererSinkImpl::OnRendererStateUpdate,
                            base::Unretained(this), true));
}

void VideoRendererSinkImpl::Stop() {
  base::AutoLock lock(callback_lock_);
  DCHECK(callback_);
  callback_ = nullptr;
  compositor_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VideoRendererSinkImpl::OnRendererStateUpdate,
                            base::Unretained(this), false));
}

void VideoRendererSinkImpl::PaintSingleFrame(
    const scoped_refptr<VideoFrame>& frame,
    bool repaint_duplicate_frame) {
  // assert(false);
}

bool VideoRendererSinkImpl::ProcessNewFrame(
    const scoped_refptr<VideoFrame>& frame,
    bool repaint_duplicate_frame) {
  DCHECK(compositor_task_runner_->BelongsToCurrentThread());

  if (frame && current_frame_ && !repaint_duplicate_frame &&
      frame->unique_id() == current_frame_->unique_id()) {
    return false;
  }

  // Set the flag indicating that the current frame is unrendered, if we get a
  // subsequent PutCurrentFrame() call it will mark it as rendered.
  rendered_last_frame_ = false;

  current_frame_ = frame;

  return true;
}

void VideoRendererSinkImpl::BackgroundRender() {
  DCHECK(compositor_task_runner_->BelongsToCurrentThread());
  const base::TimeTicks now = tick_clock_->NowTicks();
  bool new_frame = CallRender(now, now + last_interval_, true);
  if (new_frame && client_)
    client_->DidReceiveFrame(current_frame_);
}

bool VideoRendererSinkImpl::CallRender(base::TimeTicks deadline_min,
                                       base::TimeTicks deadline_max,
                                       bool background_rendering) {
  base::AutoLock lock(callback_lock_);
  if (!callback_) {
    // Even if we no longer have a callback, return true if we have a frame
    // which |client_| hasn't seen before.
    return !rendered_last_frame_ && current_frame_;
  }

  DCHECK(rendering_);

  // If the previous frame was never rendered and we're not in background
  // rendering mode (nor have just exited it), let the client know.
  if (!rendered_last_frame_ && current_frame_ && !background_rendering &&
      !is_background_rendering_) {
    callback_->OnFrameDropped();
  }

  const bool new_frame = ProcessNewFrame(
      callback_->Render(deadline_min, deadline_max, background_rendering),
      false);

  // We may create a new frame here with background rendering, but the provider
  // has no way of knowing that a new frame had been processed, so keep track of
  // the new frame, and return true on the next call to |CallRender|.
  const bool had_new_background_frame = new_background_frame_;
  new_background_frame_ = background_rendering && new_frame;

  is_background_rendering_ = background_rendering;
  last_interval_ = deadline_max - deadline_min;

  // Restart the background rendering timer whether we're background rendering
  // or not; in either case we should wait for |kBackgroundRenderingTimeoutMs|.
  if (background_rendering_enabled_)
    background_rendering_timer_.Reset();
  return new_frame || had_new_background_frame;
}

void VideoRendererSinkImpl::OnRendererStateUpdate(bool new_state) {
  DCHECK(compositor_task_runner_->BelongsToCurrentThread());
  rendering_ = new_state;
  if (rendering_)
    BackgroundRender();

  if (!client_)
    return;

  if (rendering_)
    client_->StartRendering();
  else
    client_->StopRendering();
}
}

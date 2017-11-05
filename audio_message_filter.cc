// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromium_media_lib/audio_message_filter.h"

#include <string>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "chromium_media_lib/media_context.h"

namespace media {

namespace {
const int kStreamIDNotSet = -1;
}

class AudioMessageFilter::AudioOutputIPCImpl
    : public media::AudioOutputIPC {
 public:
  AudioOutputIPCImpl(const scoped_refptr<AudioMessageFilter>& filter,
                     int render_frame_id);
  ~AudioOutputIPCImpl() override;

  // media::AudioOutputIPC implementation.
  void RequestDeviceAuthorization(media::AudioOutputIPCDelegate* delegate,
                                  int session_id,
                                  const std::string& device_id,
                                  const url::Origin& security_origin) override;
  void CreateStream(media::AudioOutputIPCDelegate* delegate,
                    const media::AudioParameters& params) override;
  void PlayStream() override;
  void PauseStream() override;
  void CloseStream() override;
  void SetVolume(double volume) override;

 private:
  const scoped_refptr<AudioMessageFilter> filter_;
  const int render_frame_id_;
  int stream_id_;
  bool stream_created_;
};

AudioMessageFilter* AudioMessageFilter::g_filter = NULL;

AudioMessageFilter::AudioMessageFilter(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : io_task_runner_(io_task_runner) {
  DCHECK(!g_filter);
  g_filter = this;
}

AudioMessageFilter::~AudioMessageFilter() {
  DCHECK_EQ(g_filter, this);
  g_filter = NULL;
}

// static
AudioMessageFilter* AudioMessageFilter::Get() {
  return g_filter;
}

AudioMessageFilter::AudioOutputIPCImpl::AudioOutputIPCImpl(
    const scoped_refptr<AudioMessageFilter>& filter,
    int render_frame_id)
    : filter_(filter),
      render_frame_id_(render_frame_id),
      stream_id_(kStreamIDNotSet),
      stream_created_(false) {
  LOG(INFO) << render_frame_id_;
}

AudioMessageFilter::AudioOutputIPCImpl::~AudioOutputIPCImpl() {}

std::unique_ptr<media::AudioOutputIPC> AudioMessageFilter::CreateAudioOutputIPC(
    int render_frame_id) {
  DCHECK_GT(render_frame_id, 0);
  return std::unique_ptr<media::AudioOutputIPC>(
      new AudioOutputIPCImpl(this, render_frame_id));
}

void AudioMessageFilter::AudioOutputIPCImpl::RequestDeviceAuthorization(
    media::AudioOutputIPCDelegate* delegate,
    int session_id,
    const std::string& device_id,
    const url::Origin& security_origin) {
  DCHECK(filter_->io_task_runner_->BelongsToCurrentThread());
  DCHECK(delegate);
  DCHECK_EQ(stream_id_, kStreamIDNotSet);
  DCHECK(!stream_created_);

  stream_id_ = filter_->delegates_.Add(delegate);
  MediaContext::Get()->audio_renderer_host()->RequestDeviceAuthorization(
      stream_id_, render_frame_id_, session_id, device_id, security_origin,
      base::Bind(&AudioMessageFilter::OnAuthorizationCompleted, filter_));
}

void AudioMessageFilter::AudioOutputIPCImpl::CreateStream(
    media::AudioOutputIPCDelegate* delegate,
    const media::AudioParameters& params) {
  DCHECK(filter_->io_task_runner_->BelongsToCurrentThread());
  DCHECK(!stream_created_);
  if (stream_id_ == kStreamIDNotSet)
    stream_id_ = filter_->delegates_.Add(delegate);

  MediaContext::Get()->audio_renderer_host()->CreateStream(
      stream_id_, render_frame_id_, params);
  stream_created_ = true;
}

void AudioMessageFilter::AudioOutputIPCImpl::PlayStream() {
  DCHECK(stream_created_);
  MediaContext::Get()->audio_renderer_host()->PlayStream(stream_id_);
}

void AudioMessageFilter::AudioOutputIPCImpl::PauseStream() {
  DCHECK(stream_created_);
}

void AudioMessageFilter::AudioOutputIPCImpl::CloseStream() {
  DCHECK(filter_->io_task_runner_->BelongsToCurrentThread());
  DCHECK_NE(stream_id_, kStreamIDNotSet);
  filter_->delegates_.Remove(stream_id_);
  stream_id_ = kStreamIDNotSet;
  stream_created_ = false;
}

void AudioMessageFilter::AudioOutputIPCImpl::SetVolume(double volume) {
  DCHECK(stream_created_);
}

void AudioMessageFilter::OnAuthorizationCompleted(
    int stream_id,
    media::OutputDeviceStatus device_status,
    const media::AudioParameters& output_params,
    const std::string& matched_device_id) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  media::AudioOutputIPCDelegate* delegate = delegates_.Lookup(stream_id);
  if (!delegate)
    return;

  delegate->OnDeviceAuthorized(device_status, output_params, matched_device_id);
}

void AudioMessageFilter::OnStreamCreated(
    int stream_id,
    base::SharedMemoryHandle handle,
    base::SyncSocket::TransitDescriptor socket_descriptor,
    uint32_t length) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  base::SyncSocket::Handle socket_handle =
      base::SyncSocket::UnwrapHandle(socket_descriptor);

  media::AudioOutputIPCDelegate* delegate = delegates_.Lookup(stream_id);
  if (!delegate) {
    DLOG(WARNING) << "Got OnStreamCreated() event for a non-existent or removed"
                  << " audio renderer. (stream_id=" << stream_id << ").";
    base::SharedMemory::CloseHandle(handle);
    base::SyncSocket socket(socket_handle);
    return;
  }
  delegate->OnStreamCreated(handle, socket_handle, length);
}

void AudioMessageFilter::OnStreamError(int stream_id) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  media::AudioOutputIPCDelegate* delegate = delegates_.Lookup(stream_id);
  if (!delegate) {
    DLOG(WARNING) << "Got OnStreamError() event for a non-existent or"
                  << " removed audio renderer.";
    return;
  }
  delegate->OnError();
}

}  // namespace media

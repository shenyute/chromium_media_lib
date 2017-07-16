// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#include "chromium_media_lib/audio_renderer_host.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "chromium_media_lib/audio_output_delegate_impl.h"
#include "chromium_media_lib/media_context.h"
#include "chromium_media_lib/media_internals.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_system.h"

namespace media {

void AudioRendererHost::SetClient(AudioRendererHostClient* client) {
  client_ = client;
}

AudioRendererHost::AudioRendererHost(AudioManager* audio_manager,
                    AudioSystem* audio_system,
                    AudioRendererHostClient* client)
    : audio_manager_(audio_manager),
      audio_system_(audio_system),
      client_(client) {
  DCHECK(audio_manager_);
}

AudioRendererHost::~AudioRendererHost() {
}

void AudioRendererHost::RequestDeviceAuthorization(int stream_id,
                                int render_frame_id,
                                int session_id,
                                const std::string& device_id,
                                const url::Origin& security_origin,
                                AuthorizationCompletedCallback cb) {
  if (AudioDeviceDescription::IsDefaultDevice(device_id)) {
    // Default device doesn't need authorization.
    authorizations_.insert(
        std::make_pair(stream_id, std::make_pair(true,
          AudioDeviceDescription::kDefaultDeviceId)));
    audio_system_->GetOutputStreamParameters(
        AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(&AudioRendererHost::DeviceParametersReceived,
          base::Unretained(this), stream_id, std::move(cb),
          AudioDeviceDescription::kDefaultDeviceId));
    return;
  }
  assert(false);
}

void AudioRendererHost::CreateStream(int stream_id,
      int render_frame_id,
      const AudioParameters& params) {
  const auto& auth_data = authorizations_.find(stream_id);
  std::string device_unique_id;
  if (auth_data != authorizations_.end()) {
    if (!auth_data->second.first) {
      OnStreamError(stream_id);
      return;
    }
    device_unique_id.swap(auth_data->second.second);
    authorizations_.erase(auth_data);
  }
  MediaInternals* const media_internals = MediaInternals::GetInstance();
  std::unique_ptr<AudioLog> audio_log = media_internals->CreateAudioLog(
      AudioLogFactory::AUDIO_OUTPUT_CONTROLLER);
  delegates_.push_back(
      base::WrapUnique<AudioOutputDelegate>(new AudioOutputDelegateImpl(
          this, audio_manager_, std::move(audio_log),
          stream_id, render_frame_id,
          params, device_unique_id, MediaContext::Get()->io_task_runner())));
}

void AudioRendererHost::PlayStream(int stream_id) {
  media::AudioOutputDelegate* delegate = LookupById(stream_id);
  if (!delegate) {
    OnStreamError(stream_id);
    return;
  }

  delegate->OnPlayStream();
}

void AudioRendererHost::DeviceParametersReceived(
  int stream_id,
  AuthorizationCompletedCallback cb,
  const std::string& raw_device_id,
  const AudioParameters& output_params) const {
DCHECK(!raw_device_id.empty());

cb.Run(stream_id, OUTPUT_DEVICE_STATUS_OK,
    output_params.IsValid() ? output_params
    : AudioParameters::UnavailableDeviceParams(),
    raw_device_id);
}

void AudioRendererHost::OnStreamCreated(
    int stream_id,
    base::SharedMemory* shared_memory,
    std::unique_ptr<base::CancelableSyncSocket> foreign_socket) {
  base::SharedMemoryHandle foreign_memory_handle;
  base::SyncSocket::TransitDescriptor socket_descriptor;
  base::ProcessHandle handle = base::GetCurrentProcessHandle();
  size_t shared_memory_size = shared_memory->requested_size();
  if (!(shared_memory->ShareToProcess(handle, &foreign_memory_handle) &&
        foreign_socket->PrepareTransitDescriptor(handle,
                                                 &socket_descriptor))) {
    // Something went wrong in preparing the IPC handles.
    client_->OnStreamError(stream_id);
    return;
  }
  // NOTE: It is important to release handle otherwise the socket_descriptor will be
  // invalide after foreign_socket destroy
  foreign_socket->Release();
  client_->OnStreamCreated(stream_id, foreign_memory_handle,
      socket_descriptor, base::checked_cast<uint32_t>(shared_memory_size));
}

void AudioRendererHost::OnStreamError(int stream_id) {
  if (client_)
    client_->OnStreamError(stream_id);
}

AudioRendererHost::AudioOutputDelegateVector::iterator
AudioRendererHost::LookupIteratorById(int stream_id) {
  return std::find_if(
      delegates_.begin(), delegates_.end(),
      [stream_id](const std::unique_ptr<AudioOutputDelegate>& d) {
        return d->GetStreamId() == stream_id;
      });
}

void AudioRendererHost::OnCloseStream(int stream_id) {
  authorizations_.erase(stream_id);

  auto i = LookupIteratorById(stream_id);

  // Prevent oustanding callbacks from attempting to close/delete the same
  // AudioOutputDelegate twice.
  if (i == delegates_.end())
    return;

  std::swap(*i, delegates_.back());
  delegates_.pop_back();
}

media::AudioOutputDelegate* AudioRendererHost::LookupById(int stream_id) {
  auto i = LookupIteratorById(stream_id);
  return i != delegates_.end() ? i->get() : nullptr;
}

} // namespace media

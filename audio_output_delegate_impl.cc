#include "chromium_media_lib/audio_output_delegate_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "chromium_media_lib/audio_sync_reader.h"
#include "media/audio/audio_output_controller.h"

namespace media {

class AudioOutputDelegateImpl::ControllerEventHandler
    : public AudioOutputController::EventHandler {
 public:
  explicit ControllerEventHandler(
      base::WeakPtr<AudioOutputDelegateImpl> delegate,
      base::SingleThreadTaskRunner* io_task_runner);

 private:
  void OnControllerCreated() override;
  void OnControllerPlaying() override;
  void OnControllerPaused() override;
  void OnControllerError() override;

  base::WeakPtr<AudioOutputDelegateImpl> delegate_;

  base::SingleThreadTaskRunner* const io_task_runner_;
};

AudioOutputDelegateImpl::ControllerEventHandler::ControllerEventHandler(
    base::WeakPtr<AudioOutputDelegateImpl> delegate,
    base::SingleThreadTaskRunner* io_task_runner)
    : delegate_(std::move(delegate)),
      io_task_runner_(io_task_runner) {}

void AudioOutputDelegateImpl::ControllerEventHandler::OnControllerCreated() {
  io_task_runner_->PostTask(FROM_HERE,
      base::Bind(&AudioOutputDelegateImpl::SendCreatedNotification, delegate_));
}

void AudioOutputDelegateImpl::ControllerEventHandler::OnControllerPlaying() {
  io_task_runner_->PostTask(FROM_HERE,
      base::Bind(&AudioOutputDelegateImpl::UpdatePlayingState, delegate_,
                 true));
}

void AudioOutputDelegateImpl::ControllerEventHandler::OnControllerPaused() {
  io_task_runner_->PostTask(FROM_HERE,
      base::Bind(&AudioOutputDelegateImpl::UpdatePlayingState, delegate_,
                 false));
}

void AudioOutputDelegateImpl::ControllerEventHandler::OnControllerError() {
  io_task_runner_->PostTask(FROM_HERE,
      base::Bind(&AudioOutputDelegateImpl::OnError, delegate_));
}

AudioOutputDelegateImpl::AudioOutputDelegateImpl(
    EventHandler* handler,
    AudioManager* audio_manager,
    std::unique_ptr<AudioLog> audio_log,
    int stream_id,
    int render_frame_id,
    const AudioParameters& params,
    const std::string& output_device_id,
    base::SingleThreadTaskRunner* io_task_runner)
    : subscriber_(handler),
      audio_log_(std::move(audio_log)),
      reader_(AudioSyncReader::Create(params)),
      stream_id_(stream_id),
      io_task_runner_(io_task_runner),
      weak_factory_(this) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(subscriber_);
  DCHECK(audio_manager);
  DCHECK(audio_log_);
  // Since the event handler never directly calls functions on |this| but rather
  // posts them to the IO thread, passing a pointer from the constructor is
  // safe.
  controller_event_handler_ =
      base::MakeUnique<ControllerEventHandler>(weak_factory_.GetWeakPtr(), io_task_runner_);
  audio_log_->OnCreated(stream_id, params, output_device_id);
  controller_ = AudioOutputController::Create(
      audio_manager, controller_event_handler_.get(), params, output_device_id,
      reader_.get());
  DCHECK(controller_);
}

AudioOutputDelegateImpl::~AudioOutputDelegateImpl() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  UpdatePlayingState(false);
  audio_log_->OnClosed(stream_id_);

  // Since the ownership of |controller_| is shared, we instead use its Close
  // method to stop callbacks from it. |controller_| will call the closure (on
  // the IO thread) when it's done closing, and it is only after that call that
  // we can delete |controller_event_handler_| and |reader_|. By giving the
  // closure ownership of these, we keep them alive until |controller_| is
  // closed.
  controller_->Close(base::Bind(
      [](std::unique_ptr<ControllerEventHandler> event_handler,
         std::unique_ptr<AudioSyncReader> reader,
         scoped_refptr<AudioOutputController> controller) {
      },
      base::Passed(&controller_event_handler_),
      base::Passed(&reader_), controller_));
}

scoped_refptr<AudioOutputController>
AudioOutputDelegateImpl::GetController() const {
  return controller_;
}

int AudioOutputDelegateImpl::GetStreamId() const {
  return stream_id_;
}

void AudioOutputDelegateImpl::OnPlayStream() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  controller_->Play();
  audio_log_->OnStarted(stream_id_);
}

void AudioOutputDelegateImpl::OnPauseStream() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  controller_->Pause();
  audio_log_->OnStopped(stream_id_);
}

void AudioOutputDelegateImpl::OnSetVolume(double volume) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK_GE(volume, 0);
  DCHECK_LE(volume, 1);
  controller_->SetVolume(volume);
  audio_log_->OnSetVolume(stream_id_, volume);
}

void AudioOutputDelegateImpl::SendCreatedNotification() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  subscriber_->OnStreamCreated(stream_id_, reader_->shared_memory(),
                               reader_->TakeForeignSocket());
}

void AudioOutputDelegateImpl::UpdatePlayingState(bool playing) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (playing == playing_)
    return;

  playing_ = playing;
}

void AudioOutputDelegateImpl::OnError() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  audio_log_->OnError(stream_id_);
  subscriber_->OnStreamError(stream_id_);
}

} // namespace media

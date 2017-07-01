#include "chromium_media_lib/audiosourceprovider_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"

namespace media {

// TeeFilter is a RenderCallback implementation that allows for a client to get
// a copy of the data being rendered by the |renderer_| on Render(). This class
// also holds on to the necessary audio parameters.
class AudioSourceProviderImpl::TeeFilter
    : public AudioRendererSink::RenderCallback {
 public:
  TeeFilter() : renderer_(nullptr), channels_(0), sample_rate_(0) {}
  ~TeeFilter() override {}

  void Initialize(AudioRendererSink::RenderCallback* renderer,
                  int channels,
                  int sample_rate) {
    DCHECK(renderer);
    renderer_ = renderer;
    channels_ = channels;
    sample_rate_ = sample_rate;
  }

  // AudioRendererSink::RenderCallback implementation.
  // These are forwarders to |renderer_| and are here to allow for a client to
  // get a copy of the rendered audio by SetCopyAudioCallback().
  int Render(base::TimeDelta delay,
             base::TimeTicks delay_timestamp,
             int prior_frames_skipped,
             AudioBus* dest) override;
  void OnRenderError() override;

  bool IsInitialized() const { return !!renderer_; }
  int channels() const { return channels_; }
  int sample_rate() const { return sample_rate_; }
  void set_copy_audio_bus_callback(const CopyAudioCB& callback) {
    copy_audio_bus_callback_ = callback;
  }

 private:
  AudioRendererSink::RenderCallback* renderer_;
  int channels_;
  int sample_rate_;

  CopyAudioCB copy_audio_bus_callback_;

  DISALLOW_COPY_AND_ASSIGN(TeeFilter);
};


AudioSourceProviderImpl::AudioSourceProviderImpl(
    scoped_refptr<SwitchableAudioRendererSink> sink,
    scoped_refptr<MediaLog> media_log)
    : volume_(1.0),
      sink_(std::move(sink)),
      tee_filter_(new TeeFilter()),
      media_log_(std::move(media_log)),
      weak_factory_(this) {}

AudioSourceProviderImpl::~AudioSourceProviderImpl() {
}

void AudioSourceProviderImpl::Initialize(const AudioParameters& params,
                        RenderCallback* renderer) {
  base::AutoLock auto_lock(sink_lock_);
  OutputDeviceStatus device_status =
      sink_ ? sink_->GetOutputDeviceInfo().device_status()
            : OUTPUT_DEVICE_STATUS_ERROR_NOT_FOUND;
  if (device_status != OUTPUT_DEVICE_STATUS_OK) {
    // Since null sink is always OK, we will fall back to it once and forever.
    if (sink_)
      sink_->Stop();
    sink_ = CreateFallbackSink();
    MEDIA_LOG(ERROR, media_log_)
        << "Output device error, falling back to null sink";
  }
  tee_filter_->Initialize(renderer, params.channels(), params.sample_rate());

  sink_->Initialize(params, tee_filter_.get());
}

void AudioSourceProviderImpl::Start() {
  base::AutoLock auto_lock(sink_lock_);
  DCHECK(tee_filter_);
  sink_->Start();
}

void AudioSourceProviderImpl::Stop() {
  base::AutoLock auto_lock(sink_lock_);
  sink_->Stop();
}

void AudioSourceProviderImpl::Play() {
  base::AutoLock auto_lock(sink_lock_);
  sink_->Play();
}

void AudioSourceProviderImpl::Pause() {
  base::AutoLock auto_lock(sink_lock_);
  sink_->Pause();
}

bool AudioSourceProviderImpl::SetVolume(double volume) {
  base::AutoLock auto_lock(sink_lock_);
  volume_ = volume;
  sink_->SetVolume(volume);
  return true;
}

OutputDeviceInfo AudioSourceProviderImpl::GetOutputDeviceInfo() {
  base::AutoLock auto_lock(sink_lock_);
  return sink_ ? sink_->GetOutputDeviceInfo()
               : OutputDeviceInfo(OUTPUT_DEVICE_STATUS_ERROR_NOT_FOUND);
}

bool AudioSourceProviderImpl::CurrentThreadIsRenderingThread() {
  NOTIMPLEMENTED();
  return false;
}

void AudioSourceProviderImpl::SwitchOutputDevice(const std::string& device_id,
                        const url::Origin& security_origin,
                        const OutputDeviceStatusCB& callback) {
  if (!sink_)
    callback.Run(OUTPUT_DEVICE_STATUS_ERROR_INTERNAL);
  else
    sink_->SwitchOutputDevice(device_id, security_origin, callback);
}

scoped_refptr<SwitchableAudioRendererSink>
AudioSourceProviderImpl::CreateFallbackSink() {
  // Assuming it is called on media thread.
  return new NullAudioSink(base::ThreadTaskRunnerHandle::Get());
}

int AudioSourceProviderImpl::TeeFilter::Render(
    base::TimeDelta delay,
    base::TimeTicks delay_timestamp,
    int prior_frames_skipped,
    AudioBus* audio_bus) {
  DCHECK(IsInitialized());

  const int num_rendered_frames = renderer_->Render(
      delay, delay_timestamp, prior_frames_skipped, audio_bus);

  if (!copy_audio_bus_callback_.is_null()) {
    const int64_t frames_delayed =
        AudioTimestampHelper::TimeToFrames(delay, sample_rate_);
    std::unique_ptr<AudioBus> bus_copy =
        AudioBus::Create(audio_bus->channels(), audio_bus->frames());
    audio_bus->CopyTo(bus_copy.get());
    copy_audio_bus_callback_.Run(std::move(bus_copy), frames_delayed,
                                 sample_rate_);
  }

  return num_rendered_frames;
}

void AudioSourceProviderImpl::TeeFilter::OnRenderError() {
  DCHECK(IsInitialized());
  renderer_->OnRenderError();
}

} // namespace media

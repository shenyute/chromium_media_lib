#include "chromium_media_lib/media_internals.h"

#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "media/base/audio_parameters.h"

namespace media {

const char kAudioLogStatusKey[] = "status";
const char kAudioLogUpdateFunction[] = "media.updateAudioComponent";

std::string EffectsToString(int effects) {
  if (effects == media::AudioParameters::NO_EFFECTS)
    return "NO_EFFECTS";

  struct {
    int flag;
    const char* name;
  } flags[] = {
    { media::AudioParameters::ECHO_CANCELLER, "ECHO_CANCELLER" },
    { media::AudioParameters::DUCKING, "DUCKING" },
    { media::AudioParameters::KEYBOARD_MIC, "KEYBOARD_MIC" },
    { media::AudioParameters::HOTWORD, "HOTWORD" },
  };

  std::string ret;
  for (size_t i = 0; i < arraysize(flags); ++i) {
    if (effects & flags[i].flag) {
      if (!ret.empty())
        ret += " | ";
      ret += flags[i].name;
      effects &= ~flags[i].flag;
    }
  }

  if (effects) {
    if (!ret.empty())
      ret += " | ";
    ret += base::IntToString(effects);
  }

  return ret;
}

std::string FormatToString(media::AudioParameters::Format format) {
  switch (format) {
    case media::AudioParameters::AUDIO_PCM_LINEAR:
      return "pcm_linear";
    case media::AudioParameters::AUDIO_PCM_LOW_LATENCY:
      return "pcm_low_latency";
    case media::AudioParameters::AUDIO_BITSTREAM_AC3:
      return "ac3";
    case media::AudioParameters::AUDIO_BITSTREAM_EAC3:
      return "eac3";
    case media::AudioParameters::AUDIO_FAKE:
      return "fake";
  }

  NOTREACHED();
  return "unknown";
}

class AudioLogImpl : public media::AudioLog {
 public:
  AudioLogImpl(int owner_id,
               media::AudioLogFactory::AudioComponent component,
               MediaInternals* media_internals);
  ~AudioLogImpl() override;

  void OnCreated(int component_id,
                 const media::AudioParameters& params,
                 const std::string& device_id) override;
  void OnStarted(int component_id) override;
  void OnStopped(int component_id) override;
  void OnClosed(int component_id) override;
  void OnError(int component_id) override;
  void OnSetVolume(int component_id, double volume) override;
  void OnSwitchOutputDevice(int component_id,
                            const std::string& device_id) override;
  void OnLogMessage(int component_id, const std::string& message) override;

 private:
  void SendSingleStringUpdate(int component_id,
                              const std::string& key,
                              const std::string& value);
  void StoreComponentMetadata(int component_id, base::DictionaryValue* dict);
  std::string FormatCacheKey(int component_id);

  const int owner_id_;
  const media::AudioLogFactory::AudioComponent component_;
  MediaInternals* const media_internals_;

  DISALLOW_COPY_AND_ASSIGN(AudioLogImpl);
};

AudioLogImpl::AudioLogImpl(int owner_id,
                           media::AudioLogFactory::AudioComponent component,
                           MediaInternals* media_internals)
    : owner_id_(owner_id),
      component_(component),
      media_internals_(media_internals) {}

AudioLogImpl::~AudioLogImpl() {}

void AudioLogImpl::OnCreated(int component_id,
                             const media::AudioParameters& params,
                             const std::string& device_id) {
  base::DictionaryValue dict;
  StoreComponentMetadata(component_id, &dict);

  dict.SetString(kAudioLogStatusKey, "created");
  dict.SetString("device_id", device_id);
  dict.SetString("device_type", FormatToString(params.format()));
  dict.SetInteger("frames_per_buffer", params.frames_per_buffer());
  dict.SetInteger("sample_rate", params.sample_rate());
  dict.SetInteger("channels", params.channels());
  dict.SetString("channel_layout",
                 ChannelLayoutToString(params.channel_layout()));
  dict.SetString("effects", EffectsToString(params.effects()));

  media_internals_->UpdateAudioLog(MediaInternals::CREATE,
                                   FormatCacheKey(component_id),
                                   kAudioLogUpdateFunction, &dict);
}

void AudioLogImpl::OnStarted(int component_id) {
  SendSingleStringUpdate(component_id, kAudioLogStatusKey, "started");
}

void AudioLogImpl::OnStopped(int component_id) {
  SendSingleStringUpdate(component_id, kAudioLogStatusKey, "stopped");
}

void AudioLogImpl::OnClosed(int component_id) {
  base::DictionaryValue dict;
  StoreComponentMetadata(component_id, &dict);
  dict.SetString(kAudioLogStatusKey, "closed");
  media_internals_->UpdateAudioLog(MediaInternals::UPDATE_AND_DELETE,
                                   FormatCacheKey(component_id),
                                   kAudioLogUpdateFunction, &dict);
}

void AudioLogImpl::OnError(int component_id) {
  SendSingleStringUpdate(component_id, "error_occurred", "true");
}

void AudioLogImpl::OnSetVolume(int component_id, double volume) {
  base::DictionaryValue dict;
  StoreComponentMetadata(component_id, &dict);
  dict.SetDouble("volume", volume);
  media_internals_->UpdateAudioLog(MediaInternals::UPDATE_IF_EXISTS,
                                   FormatCacheKey(component_id),
                                   kAudioLogUpdateFunction, &dict);
}

void AudioLogImpl::OnSwitchOutputDevice(int component_id,
                                        const std::string& device_id) {
  base::DictionaryValue dict;
  StoreComponentMetadata(component_id, &dict);
  dict.SetString("device_id", device_id);
  media_internals_->UpdateAudioLog(MediaInternals::UPDATE_IF_EXISTS,
                                   FormatCacheKey(component_id),
                                   kAudioLogUpdateFunction, &dict);
}

void AudioLogImpl::OnLogMessage(int component_id, const std::string& message) {
  // TODO
}

std::string AudioLogImpl::FormatCacheKey(int component_id) {
  return base::StringPrintf("%d:%d:%d", owner_id_, component_, component_id);
}

void AudioLogImpl::SendSingleStringUpdate(int component_id,
                                          const std::string& key,
                                          const std::string& value) {
  base::DictionaryValue dict;
  StoreComponentMetadata(component_id, &dict);
  dict.SetString(key, value);
  media_internals_->UpdateAudioLog(MediaInternals::UPDATE_IF_EXISTS,
                                   FormatCacheKey(component_id),
                                   kAudioLogUpdateFunction, &dict);
}

void AudioLogImpl::StoreComponentMetadata(int component_id,
                                          base::DictionaryValue* dict) {
  dict->SetInteger("owner_id", owner_id_);
  dict->SetInteger("component_id", component_id);
  dict->SetInteger("component_type", component_);
}

MediaInternals* MediaInternals::GetInstance() {
  static MediaInternals* internals = new MediaInternals();
  return internals;
}

MediaInternals::MediaInternals()
    : owner_ids_() {
}

MediaInternals::~MediaInternals() {}


std::unique_ptr<AudioLog> MediaInternals::CreateAudioLog(
    AudioComponent component) {
  base::AutoLock auto_lock(lock_);
  return std::unique_ptr<media::AudioLog>(
      new AudioLogImpl(owner_ids_[component]++, component, this));
}

void MediaInternals::UpdateAudioLog(AudioLogUpdateType type,
                                    const std::string& cache_key,
                                    const std::string& function,
                                    const base::DictionaryValue* value) {
  // TODO
}

} // namespace media

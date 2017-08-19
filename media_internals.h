// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
//

#ifndef CHROMIUM_MEDIA_LIB_MEDIA_INTERNALS_H_
#define CHROMIUM_MEDIA_LIB_MEDIA_INTERNALS_H_

#include <list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "media/audio/audio_logging.h"
#include "media/base/media_export.h"
#include "media/base/media_log.h"

namespace media {
struct MediaLogEvent;

// This class stores information about currently active media.
class MEDIA_EXPORT MediaInternals
    : NON_EXPORTED_BASE(public media::AudioLogFactory) {
 public:
  static MediaInternals* GetInstance();

  ~MediaInternals() override;

  // AudioLogFactory implementation.  Safe to call from any thread.
  std::unique_ptr<AudioLog> CreateAudioLog(AudioComponent component) override;

 private:
  friend class AudioLogImpl;
  enum AudioLogUpdateType {
    CREATE,             // Creates a new AudioLog cache entry.
    UPDATE_IF_EXISTS,   // Updates an existing AudioLog cache entry, does
                        // nothing if it doesn't exist.
    UPDATE_AND_DELETE,  // Deletes an existing AudioLog cache entry.
  };
  void UpdateAudioLog(AudioLogUpdateType type,
                      const std::string& cache_key,
                      const std::string& function,
                      const base::DictionaryValue* value);

  MediaInternals();

  // All variables below must be accessed under |lock_|.
  base::Lock lock_;
  base::DictionaryValue audio_streams_cached_data_;
  int owner_ids_[AUDIO_COMPONENT_MAX];

  DISALLOW_COPY_AND_ASSIGN(MediaInternals);
};

}  // namespace media

#endif  // CHROMIUM_MEDIA_LIB_MEDIA_INTERNALS_H_

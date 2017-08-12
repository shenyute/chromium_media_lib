// Copyright (c) 2017 YuTeh Shen
//
#ifndef CHROMIUM_MEDIA_LIB_RESOURCE_DATA_SOURCE_H_
#define CHROMIUM_MEDIA_LIB_RESOURCE_DATA_SOURCE_H_

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "chromium_media_lib/resource_multibuffer.h"
#include "chromium_media_lib/read_operation.h"
#include "media/base/data_source.h"
#include "url/gurl.h"

namespace media {

class ResourceDataSource : public DataSource,
    public ResourceMultiBufferClient {
 public:
   ResourceDataSource(const GURL& url,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
   ~ResourceDataSource() override;

  typedef base::Callback<void(bool)> InitializeCB;
  void Initialize(const InitializeCB& init_cb);

  // DataSource implementation.
  // Called from demuxer thread.
  void Stop() override;
  void Abort() override;

  void Read(int64_t position,
            int size,
            uint8_t* data,
            const DataSource::ReadCB& read_cb) override;
  bool GetSize(int64_t* size_out) override;
  bool IsStreaming() override;
  void SetBitrate(int bitrate) override;

  // ResourceMultiBufferClient
  void DidInitialize() override;
  void OnUpdateState() override;

 private:
  void ReadTask();

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> render_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  GURL url_;

  base::Lock lock_;
  bool stop_signal_received_;
  int64_t total_bytes_;

  InitializeCB init_cb_;
  ResourceMultiBuffer multibuffer_;

  std::unique_ptr<ReadOperation> read_op_;

  base::WeakPtr<ResourceDataSource> weak_ptr_;
  base::WeakPtrFactory<ResourceDataSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ResourceDataSource);
};

} // namespace media

#endif // CHROMIUM_MEDIA_LIB_RESOURCE_DATA_SOURCE_H_

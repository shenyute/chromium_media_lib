// Copyright (c) 2017 YuTeh Shen
//
#ifndef CHROMIUM_MEDIA_LIB_RESOURCE_DATA_SOURCE_H_
#define CHROMIUM_MEDIA_LIB_RESOURCE_DATA_SOURCE_H_

#include <memory>

#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "media/base/data_source.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace media {

class ResourceDataSource : public DataSource,
    public net::URLFetcherDelegate {
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

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Invoked by URLFetcherResponseWriter
  void DidInitialize();

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> render_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  GURL url_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  InitializeCB init_cb_;

  DISALLOW_COPY_AND_ASSIGN(ResourceDataSource);
};

} // namespace media

#endif // CHROMIUM_MEDIA_LIB_RESOURCE_DATA_SOURCE_H_

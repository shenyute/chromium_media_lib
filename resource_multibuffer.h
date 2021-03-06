// Copyright (c) 2017 YuTeh Shen
//
#ifndef CHROMIUM_MEDIA_LIB_RESOURCE_MULTIBUFFER_H_
#define CHROMIUM_MEDIA_LIB_RESOURCE_MULTIBUFFER_H_

#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "chromium_media_lib/lru.h"
#include "media/base/data_buffer.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#include <map>
#include <memory>
#include <utility>

namespace media {

typedef int32_t MultiBufferBlockId;

class ResourceMultiBufferClient {
 public:
  virtual void DidInitialize() = 0;
  virtual void OnUpdateState() = 0;
};

// Use URLFetcher to buffer network resource.
class ResourceMultiBuffer : public net::URLFetcherDelegate {
 public:
  ResourceMultiBuffer(
      ResourceMultiBufferClient* client,
      const GURL& url,
      int32_t block_size_shift,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
  ~ResourceMultiBuffer() override;

  MultiBufferBlockId ToBlockId(int position);

  int64_t GetSize();
  void Start();
  void Seek(int position);
  // Try to fill data into |data|, and return write bytes or
  // net::ERR_IO_PENDINGO if no data available now.
  int Fill(int position, int size, void* data);

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Invoked by URLFetcherResponseWriter
  void DidInitialize();
  int OnWrite(net::IOBuffer* buffer,
              int num_bytes,
              const net::CompletionCallback& callback);
  int OnFinish(int net_error, const net::CompletionCallback& callback);

 private:
  void AdjustPinnedRange(MultiBufferBlockId id);
  void CreateFetcherFrom(int position);
  void PurgeIfNecessary();
  bool CheckCacheMiss(int position);

 private:
  GURL url_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  int write_start_pos_;
  int write_offset_;
  int64_t total_bytes_;
  ResourceMultiBufferClient* client_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  base::Lock lock_;
  int32_t block_size_shift_;

  // cache relative
  std::map<MultiBufferBlockId, scoped_refptr<DataBuffer>> cache_;
  LRU<MultiBufferBlockId> lru_;
  // region which should not be pruned by LRU
  std::pair<MultiBufferBlockId, MultiBufferBlockId> pinned_range_;
};
}

#endif  // CHROMIUM_MEDIA_LIB_RESOURCE_MULTIBUFFER_H_

#include "chromium_media_lib/resource_multibuffer.h"

#include "base/lazy_instance.h"
#include "net/http/http_response_headers.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

namespace media {

static int kMaxWaitForReaderOffset = 512 * 1024;  // 512 kb
static const int kHttpPartialContent = 206;

struct RequestContextInitializer {
  RequestContextInitializer() {
    net::URLRequestContextBuilder builder;
    builder.set_data_enabled(true);
    builder.set_file_enabled(true);
    builder.set_proxy_config_service(base::WrapUnique(
        new net::ProxyConfigServiceFixed(net::ProxyConfig::CreateDirect())));
    url_request_context_ = builder.Build();
  }

  ~RequestContextInitializer() {}

  net::URLRequestContext* request_context() {
    return url_request_context_.get();
  }

  std::unique_ptr<net::URLRequestContext> url_request_context_;
};

class URLFetcherResponseWriterBridge : public net::URLFetcherResponseWriter {
 public:
  URLFetcherResponseWriterBridge(ResourceMultiBuffer* buffer)
      : buffer_(buffer) {}

  ~URLFetcherResponseWriterBridge() override {}

  int Initialize(const net::CompletionCallback& callback) override {
    buffer_->DidInitialize();
    return 0;
  }

  int Write(net::IOBuffer* buffer,
            int num_bytes,
            const net::CompletionCallback& callback) override {
    return buffer_->OnWrite(buffer, num_bytes, callback);
  }

  int Finish(int net_error, const net::CompletionCallback& callback) override {
    buffer_->OnFinish(net_error, callback);
    return 0;
  }

 private:
  ResourceMultiBuffer* buffer_;
};

static base::LazyInstance<RequestContextInitializer>::Leaky
    g_request_context_init = LAZY_INSTANCE_INITIALIZER;

ResourceMultiBuffer::ResourceMultiBuffer(
    ResourceMultiBufferClient* client,
    const GURL& url,
    int32_t block_size_shift,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : url_(url),
      io_task_runner_(io_task_runner),
      write_start_pos_(0),
      write_offset_(0),
      total_bytes_(-1),
      client_(client),
      block_size_shift_(block_size_shift) {}

ResourceMultiBuffer::~ResourceMultiBuffer() {}

void ResourceMultiBuffer::Start() {
  DCHECK(!fetcher_);
  if (!io_task_runner_->BelongsToCurrentThread()) {
    io_task_runner_->PostTask(FROM_HERE, base::Bind(&ResourceMultiBuffer::Start,
                                                    base::Unretained(this)));
    return;
  }
  base::AutoLock auto_lock(lock_);
  CreateFetcherFrom(0);
}

MultiBufferBlockId ResourceMultiBuffer::ToBlockId(int position) {
  return position >> block_size_shift_;
}

int64_t ResourceMultiBuffer::GetSize() {
  base::AutoLock auto_lock(lock_);
  if (!fetcher_)
    return -1;
  return total_bytes_;
}

bool ResourceMultiBuffer::CheckCacheMiss(int position) {
  MultiBufferBlockId id = ToBlockId(position);
  auto i = cache_.upper_bound(id);
  bool write_behind = write_start_pos_ + write_offset_ > position;
  // the writer write entry is being purged
  if (!write_behind)
    return false;
  if (i == cache_.begin())
    return true;
  --i;
  bool no_cache_entry =
      !((i->first << block_size_shift_) + i->second->data_size() > position);
  return no_cache_entry && write_behind;
}

void ResourceMultiBuffer::Seek(int position) {
  base::AutoLock auto_lock(lock_);
  int current_write_pos = write_start_pos_ + write_offset_;
  MultiBufferBlockId id = ToBlockId(position);
  if (position < write_start_pos_ ||
      position - kMaxWaitForReaderOffset > current_write_pos ||
      CheckCacheMiss(position)) {
    id = std::max(id - 1, 0);
    write_start_pos_ = id << block_size_shift_;
    write_offset_ = 0;
    CreateFetcherFrom(write_start_pos_);
  }
  // otherwise it is ok to acces data or wait data received
}

int ResourceMultiBuffer::Fill(int position, int size, void* data) {
  base::AutoLock auto_lock(lock_);
  MultiBufferBlockId id = ToBlockId(position);
  const int buffer_size = 1 << block_size_shift_;
  auto it = cache_.upper_bound(id);
  DCHECK(it == cache_.end() || (it->first << block_size_shift_) > position);
  if (it == cache_.begin()) {
    return net::ERR_IO_PENDING;
  }
  --it;
  int write_bytes = 0;
  int index = it->first;
  DCHECK_LE(it->first << block_size_shift_, position);
  while (it != cache_.end() && size > 0 &&
         ((it->first << block_size_shift_) + it->second->data_size() >=
          position) &&
         index == it->first) {
    // copy buffer
    const int start_position = position - (it->first << block_size_shift_);
    const int remain_size =
        std::min(it->second->data_size() - start_position, size);
    DCHECK(remain_size >= 0);
    uint8_t* p = (uint8_t*)data + write_bytes;
    memcpy(p, it->second->data() + start_position, remain_size);
    size -= remain_size;
    position += remain_size;
    write_bytes += remain_size;
    // the buffer is not full, so need to wait until ready.
    if (it->second->data_size() != buffer_size)
      break;
    ++it;
    ++index;
  }

  return write_bytes > 0 ? write_bytes : net::ERR_IO_PENDING;
}

void ResourceMultiBuffer::OnURLFetchComplete(const net::URLFetcher* source) {
  LOG(INFO) << "OnURLFetchComplete source=" << source
            << " fetcher_=" << fetcher_.get();
}

void ResourceMultiBuffer::DidInitialize() {
  client_->DidInitialize();
}

int ResourceMultiBuffer::OnWrite(net::IOBuffer* buffer,
                                 int num_bytes,
                                 const net::CompletionCallback& callback) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  // int written = net::ERR_ABORTED;
  // http 2XX
  int written_bytes = num_bytes;
  LOG(INFO) << "write_offset=" << write_offset_ << " num_bytes=" << num_bytes;
  {
    base::AutoLock auto_lock(lock_);
    bool partial_response = fetcher_->GetResponseCode() == kHttpPartialContent;
    if (fetcher_->GetResponseCode() / 100 == 2) {
      if (partial_response) {
        net::HttpResponseHeaders* headers = fetcher_->GetResponseHeaders();
        int64_t first_byte_pos, last_byte_pos, instance_length;
        bool success = headers->GetContentRangeFor206(
            &first_byte_pos, &last_byte_pos, &instance_length);
        if (success)
          total_bytes_ = instance_length;
      } else {
        total_bytes_ = fetcher_->GetReceivedResponseContentLength();
      }
      MultiBufferBlockId id = ToBlockId(write_start_pos_ + write_offset_);
      const int buffer_size = 1 << block_size_shift_;
      char* read_data = buffer->data();
      while (num_bytes > 0) {
        scoped_refptr<DataBuffer> entry;
        auto found = cache_.find(id);
        if (found == cache_.end()) {
          entry = new DataBuffer(buffer_size);
          cache_[id] = entry;
        } else {
          entry = found->second;
        }
        int write_start = ((write_start_pos_ + write_offset_) &
                           ((1 << block_size_shift_) - 1));
        int remain_size = std::min(buffer_size - write_start, num_bytes);
        memcpy(entry->writable_data() + write_start, read_data, remain_size);
        entry->set_data_size(write_start + remain_size);
        LOG(INFO) << "id=" << id
                  << " data_size=" << (write_start + remain_size);
        read_data += remain_size;
        write_offset_ += remain_size;
        num_bytes -= remain_size;
        id++;
      }
    }
  }
  // client_->OnUpdateState();
  return written_bytes;
}

int ResourceMultiBuffer::OnFinish(int net_error,
                                  const net::CompletionCallback& callback) {
  LOG(INFO) << "ResourceDataSource::OnFinish error=" << net_error;
  client_->OnUpdateState();
  return 0;
}

void ResourceMultiBuffer::CreateFetcherFrom(int position) {
  fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this);
  fetcher_->SetRequestContext(new net::TrivialURLRequestContextGetter(
      g_request_context_init.Pointer()->request_context(), io_task_runner_));
  std::unique_ptr<net::URLFetcherResponseWriter> response_writer =
      base::MakeUnique<URLFetcherResponseWriterBridge>(this);
  fetcher_->SetExtraRequestHeaders(
      "Accept-Encoding: identity;q=1, *;q=0\r\nUser-Agent:Mozilla/5.0 (X11; "
      "Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
      "Chrome/59.0.3071.115 Safari/537.36\r\n");
  fetcher_->SaveResponseWithWriter(std::move(response_writer));
  std::stringstream range_header;
  write_start_pos_ = position;
  write_offset_ = 0;
  range_header << "Range: "
               << "bytes=" << position << "-";
  LOG(INFO) << "CreateFetcherFrom range=" << range_header.str();
  fetcher_->AddExtraRequestHeader(range_header.str());
  fetcher_->Start();
}

}  // namespace media

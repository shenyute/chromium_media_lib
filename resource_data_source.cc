#include "chromium_media_lib/resource_data_source.h"
#include "base/lazy_instance.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

namespace media {

struct RequestContextInitializer {
  RequestContextInitializer() {
    net::URLRequestContextBuilder builder;
    builder.set_data_enabled(true);
    builder.set_file_enabled(true);
    url_request_context_ = builder.Build();
  }

  ~RequestContextInitializer() {
  }

  net::URLRequestContext* request_context() {
    return url_request_context_.get();
  }

  std::unique_ptr<net::URLRequestContext> url_request_context_;
};

class URLFetcherResponseWriterBridge : public net::URLFetcherResponseWriter {
 public:
  URLFetcherResponseWriterBridge(ResourceDataSource* source)
      : source_(source) {
  }

  ~URLFetcherResponseWriterBridge() override {
  }

  int Initialize(const net::CompletionCallback& callback) override {
    source_->DidInitialize();
    return 0;
  }

  int Write(net::IOBuffer* buffer,
            int num_bytes,
            const net::CompletionCallback& callback) override {
    return 0;
  }

  int Finish(int net_error, const net::CompletionCallback& callback) override {
    return 0;
  }

 private:
  ResourceDataSource* source_;
};

static base::LazyInstance<RequestContextInitializer>::Leaky g_request_context_init =
    LAZY_INSTANCE_INITIALIZER;

ResourceDataSource::ResourceDataSource(const GURL& url,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : render_task_runner_(task_runner),
      io_task_runner_(io_task_runner),
      url_(url) {
}

ResourceDataSource::~ResourceDataSource() {
}

void ResourceDataSource::Initialize(const InitializeCB& init_cb) {
  DCHECK(render_task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());
  fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this);
  fetcher_->SetRequestContext(new net::TrivialURLRequestContextGetter(
      g_request_context_init.Pointer()->request_context(), io_task_runner_));
  std::unique_ptr<net::URLFetcherResponseWriter> response_writer =
      base::MakeUnique<URLFetcherResponseWriterBridge>(this);
  fetcher_->SaveResponseWithWriter(std::move(response_writer));
  init_cb_ = init_cb;
}

void ResourceDataSource::Stop() {
}

void ResourceDataSource::Abort() {
}


void ResourceDataSource::Read(int64_t position,
          int size,
          uint8_t* data,
          const DataSource::ReadCB& read_cb) {
}

bool ResourceDataSource::GetSize(int64_t* size_out) {
  *size_out = 0;
  return false;
}

bool ResourceDataSource::IsStreaming() {
  return false;
}

void ResourceDataSource::SetBitrate(int bitrate) {
}

void ResourceDataSource::OnURLFetchComplete(const net::URLFetcher* source) {
}

void ResourceDataSource::DidInitialize() {
  DCHECK(!init_cb_.is_null());
  render_task_runner_->PostTask(
      FROM_HERE, base::Bind(base::ResetAndReturn(&init_cb_), true));
}

} // namespace media

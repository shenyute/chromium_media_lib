#include "chromium_media_lib/resource_data_source.h"

#include "base/callback_helpers.h"
#include "net/base/net_errors.h"

namespace media {

ResourceDataSource::ResourceDataSource(const GURL& url,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : render_task_runner_(task_runner),
      io_task_runner_(io_task_runner),
      url_(url),
      stop_signal_received_(false),
      total_bytes_(0),
      multibuffer_(this, url, io_task_runner_),
      weak_factory_(this) {
  weak_ptr_ = weak_factory_.GetWeakPtr();
}

ResourceDataSource::~ResourceDataSource() {
}

void ResourceDataSource::Initialize(const InitializeCB& init_cb) {
  DCHECK(render_task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());
  init_cb_ = init_cb;
  multibuffer_.Start();
}

void ResourceDataSource::Stop() {
  base::AutoLock auto_lock(lock_);
  stop_signal_received_ = true;
}

void ResourceDataSource::Abort() {
  base::AutoLock auto_lock(lock_);
  stop_signal_received_ = true;
}


void ResourceDataSource::Read(int64_t position,
          int size,
          uint8_t* data,
          const DataSource::ReadCB& read_cb) {
  DCHECK(!read_cb.is_null());
  DCHECK(!read_op_);
  {
    base::AutoLock auto_lock(lock_);
    if (stop_signal_received_) {
      read_cb.Run(kReadError);
      return;
    }
  }
  read_op_.reset(new ReadOperation(position, size, data, read_cb));
  LOG(INFO) << "ResourceDataSource::Read position=" << read_op_->position()
      << " size=" << read_op_->size();
  render_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&ResourceDataSource::ReadTask, weak_factory_.GetWeakPtr()));
}

bool ResourceDataSource::GetSize(int64_t* size_out) {
  base::AutoLock auto_lock(lock_);

  total_bytes_ = multibuffer_.GetSize();
  if (total_bytes_ != -1) {
    *size_out = total_bytes_;
    return true;
  }
  *size_out = 0;
  return false;
}

bool ResourceDataSource::IsStreaming() {
  return false;
}

void ResourceDataSource::SetBitrate(int bitrate) {
  // Do nothing
}

void ResourceDataSource::ReadTask() {
  DCHECK(render_task_runner_->BelongsToCurrentThread());
  base::AutoLock auto_lock(lock_);
  if (stop_signal_received_ || !read_op_)
    return;
  DCHECK(read_op_->size());
  multibuffer_.Seek(read_op_->position());
  int bytes_read = multibuffer_.Fill(read_op_->position(), read_op_->size(),
      read_op_->data());
  if (bytes_read > 0) {
    ReadOperation::Run(std::move(read_op_), bytes_read);
  } else if (bytes_read == net::ERR_IO_PENDING) {
    // wait until OnUpdateState
    render_task_runner_->PostDelayedTask(FROM_HERE,
        base::Bind(&ResourceDataSource::ReadTask, weak_factory_.GetWeakPtr()),
            base::TimeDelta::FromMilliseconds(1000));
  } else {
    ReadOperation::Run(std::move(read_op_), kReadError);
  }
}

void ResourceDataSource::DidInitialize() {
  DCHECK(!init_cb_.is_null());
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  base::AutoLock auto_lock(lock_);
  render_task_runner_->PostTask(
      FROM_HERE, base::Bind(base::ResetAndReturn(&init_cb_), true));
}

void ResourceDataSource::OnUpdateState() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  render_task_runner_->PostTask(FROM_HERE,
      base::Bind(&ResourceDataSource::ReadTask, weak_factory_.GetWeakPtr()));
}

} // namespace media

#include "chromium_media_lib/file_data_source.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/macros.h"

namespace media {

class FileDataSource::ReadOperation {
 public:
  ReadOperation(int64_t position,
                int size,
                uint8_t* data,
                const DataSource::ReadCB& callback);
  ~ReadOperation();

  // Runs |callback_| with the given |result|, deleting the operation
  // afterwards.
  static void Run(std::unique_ptr<ReadOperation> read_op, int result);

  int64_t position() { return position_; }
  int size() { return size_; }
  uint8_t* data() { return data_; }

 private:
  const int64_t position_;
  const int size_;
  uint8_t* data_;
  DataSource::ReadCB callback_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ReadOperation);
};

FileDataSource::ReadOperation::ReadOperation(
    int64_t position,
    int size,
    uint8_t* data,
    const DataSource::ReadCB& callback)
    : position_(position), size_(size), data_(data), callback_(callback) {
  DCHECK(!callback_.is_null());
}

FileDataSource::ReadOperation::~ReadOperation() {
  DCHECK(callback_.is_null());
}

// static
void FileDataSource::ReadOperation::Run(
    std::unique_ptr<ReadOperation> read_op,
    int result) {
  base::ResetAndReturn(&read_op->callback_).Run(result);
}

FileDataSource::FileDataSource(const base::FilePath& path,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : render_task_runner_(task_runner),
      path_(path),
      total_bytes_(-1),
      stop_signal_received_(false),
      weak_factory_(this) {
  weak_ptr_ = weak_factory_.GetWeakPtr();
}

FileDataSource::~FileDataSource() {
}


void FileDataSource::Initialize(const InitializeCB& init_cb) {
  DCHECK(render_task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());
  bool success = mapped_file_.Initialize(base::File(path_,
      base::File::FLAG_OPEN | base::File::FLAG_READ));
  {
    base::AutoLock auto_lock(lock_);
    if (success)
      total_bytes_ = mapped_file_.length();
  }
  init_cb_ = init_cb;
  render_task_runner_->PostTask(
      FROM_HERE, base::Bind(base::ResetAndReturn(&init_cb_), success));
}

void FileDataSource::Stop() {
  base::AutoLock auto_lock(lock_);
  stop_signal_received_ = true;
}

void FileDataSource::Abort() {
  base::AutoLock auto_lock(lock_);
  stop_signal_received_ = true;
}

void FileDataSource::Read(int64_t position,
          int size,
          uint8_t* data,
          const DataSource::ReadCB& read_cb) {
  DCHECK(!read_cb.is_null());
  {
    base::AutoLock auto_lock(lock_);
    DCHECK(!read_op_);

    if (stop_signal_received_) {
      read_cb.Run(kReadError);
      return;
    }

    read_op_.reset(new ReadOperation(position, size, data, read_cb));
  }
  render_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FileDataSource::ReadTask, weak_factory_.GetWeakPtr()));
}

bool FileDataSource::GetSize(int64_t* size_out) {
  base::AutoLock auto_lock(lock_);
  if (total_bytes_ != -1) {
    *size_out = total_bytes_;
    return true;
  }
  *size_out = 0;
  return false;
}

bool FileDataSource::IsStreaming() {
  return false;
}

void FileDataSource::SetBitrate(int bitrate) {
  // Do nothing
}

void FileDataSource::ReadTask() {
  DCHECK(render_task_runner_->BelongsToCurrentThread());
  base::AutoLock auto_lock(lock_);
  if (stop_signal_received_ || !read_op_)
    return;

  DCHECK(read_op_->size());
  if (mapped_file_.IsValid()) {
    const uint8_t* file_data = mapped_file_.data();
    int64_t available = total_bytes_ - read_op_->position();
    if (available > 0) {
      int bytes_read =
          static_cast<int>(std::min<int64_t>(available,
                static_cast<int64_t>(read_op_->size())));
      memcpy(read_op_->data(), file_data + read_op_->position(), bytes_read);
      ReadOperation::Run(std::move(read_op_), bytes_read);
      return;
    }
  }
  ReadOperation::Run(std::move(read_op_), kReadError);
}

}  // namespace media

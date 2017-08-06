#include "chromium_media_lib/read_operation.h"

#include "base/callback_helpers.h"

namespace media {

ReadOperation::ReadOperation(
    int64_t position,
    int size,
    uint8_t* data,
    const DataSource::ReadCB& callback)
    : position_(position), size_(size), data_(data), callback_(callback) {
  DCHECK(!callback_.is_null());
}

ReadOperation::~ReadOperation() {
  DCHECK(callback_.is_null());
}

// static
void ReadOperation::Run(
    std::unique_ptr<ReadOperation> read_op,
    int result) {
  base::ResetAndReturn(&read_op->callback_).Run(result);
}

} // namespace media

// Copyright (c) 2017 YuTeh Shen
//
#ifndef CHROMIUM_MEDIA_LIB_READ_OPERATION_H_
#define CHROMIUM_MEDIA_LIB_READ_OPERATION_H_

#include "media/base/data_source.h"

#include <memory>

namespace media {

class ReadOperation {
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

}  // namespace media

#endif  // CHROMIUM_MEDIA_LIB_READ_OPERATION_H_

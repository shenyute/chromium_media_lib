#ifndef CHROMIUM_MEDIA_LIB_FILE_DATA_SOURCE_H_
#define CHROMIUM_MEDIA_LIB_FILE_DATA_SOURCE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "media/base/data_source.h"

namespace media {

class FileDataSource : public DataSource {
 public:
   FileDataSource(const base::FilePath& path,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
   ~FileDataSource() override;

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

 private:
  void ReadTask();

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> render_task_runner_;
  base::FilePath path_;
  base::MemoryMappedFile mapped_file_;
  int64_t total_bytes_;
  base::Lock lock_;
  bool stop_signal_received_;
  InitializeCB init_cb_;

  class ReadOperation;
  std::unique_ptr<ReadOperation> read_op_;

  base::WeakPtr<FileDataSource> weak_ptr_;
  base::WeakPtrFactory<FileDataSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileDataSource);
};

}  // namespace media

#endif // CHROMIUM_MEDIA_LIB_FILE_DATA_SOURCE_H_

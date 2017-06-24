#include "chromium_media_lib/mediaplayer_impl.h"

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"

int main()
{
  base::MessageLoopForUI* ui = new base::MessageLoopForUI();
  std::unique_ptr<base::Thread> media_thread(new base::Thread("Media"));
  media_thread->Start();
  std::unique_ptr<base::Thread> worker_thread(new base::Thread("Worker"));
  worker_thread->Start();
  std::unique_ptr<media::MediaLog> media_log =
    base::MakeUnique<media::MediaLog>();
  media::MediaPlayerParams params(ui->task_runner(),
      media_thread->task_runner(),
      worker_thread->task_runner(),
      std::move(media_log));
  std::unique_ptr<media::MediaPlayerImpl> player =
      base::MakeUnique<media::MediaPlayerImpl>(params);
  return 0;
}

#include "chromium_media_lib/mediaplayer_impl.h"

#include <memory>
#include <string>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "base/run_loop.h"
#include "chromium_media_lib/media_context.h"
#include "url/gurl.h"

class VideoFrameObserver
    : public media::VideoRendererSinkClient {
 public:
  void StartRendering() override {
    LOG(INFO) << "Start rendering video frame";
  }

  void StopRendering() override {
    LOG(INFO) << "Stop rendering video frame";
  }

  void DidReceiveFrame(scoped_refptr<media::VideoFrame> frame) override {
    LOG(INFO) << "Receive new frame!";
  }
};

struct MainParams {
  std::string media_file_;
  GURL resource_file_;
  std::unique_ptr<base::Thread> media_thread;
  std::unique_ptr<base::Thread> io_thread;
  std::unique_ptr<base::Thread> worker_thread;
  std::unique_ptr<media::MediaPlayerImpl> player;
  std::unique_ptr<VideoFrameObserver> video_renderer_;
};

void init(MainParams* params) {
  std::unique_ptr<base::TaskScheduler::InitParams> task_scheduler_init_params =
      media::MediaContext::Get()->GetDefaultTaskSchedulerInitParams();
  base::TaskScheduler::CreateAndSetDefaultTaskScheduler("renderer",
      *task_scheduler_init_params.get());
  scoped_refptr<media::MediaLog> media_log = new media::MediaLog();
  media::MediaPlayerParams media_params(base::MessageLoop::current()->task_runner(),
      params->media_thread->task_runner(),
      params->io_thread->task_runner(),
      params->worker_thread->task_runner(),
      std::move(media_log));
  media_params.SetVideoRendererSinkClient(params->video_renderer_.get());
  params->player =
      base::MakeUnique<media::MediaPlayerImpl>(media_params);
  if (params->media_file_.empty())
    params->player->Load(params->resource_file_);
  else
    params->player->Load(base::FilePath(params->media_file_));
  params->player->SetRate(1.0);
  params->player->Play();
}

int main(int argc, const char* argv[]) {
  base::AtExitManager manager;
  base::CommandLine::Init(argc, argv);
  const char media_file[] = "media-file";
  const char resource_file[] = "resource-file";

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(media_file) && !command_line->HasSwitch(resource_file)) {
    LOG(INFO) << "Usage:\n ./media_example --media-file=<file full path>";
    return 0;
  }
  MainParams params;
  if (command_line->HasSwitch(media_file))
    params.media_file_ = command_line->GetSwitchValueASCII(media_file);
  else
    params.resource_file_ = GURL(command_line->GetSwitchValueASCII(resource_file));
  params.media_thread.reset(new base::Thread("Media"));
  params.io_thread.reset(new base::Thread("IO"));
  params.worker_thread.reset(new base::Thread("Worker"));
  params.media_thread->Start();
  params.io_thread->StartWithOptions(base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  params.worker_thread->Start();
  params.video_renderer_.reset(new VideoFrameObserver);

  base::MessageLoopForUI message_loop;

  message_loop.task_runner()->PostTask(FROM_HERE, base::Bind(&init, &params));
  base::RunLoop().Run();
  return 0;
}

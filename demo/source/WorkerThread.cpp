#include <WorkerThread.h>
#include <WindowsPlatform.h>
#include <future>

namespace fraze
{

WorkerThread::WorkerThread()
{
    std::promise<void> ready;
    auto readyFuture = ready.get_future();

    worker = std::thread([this, p = std::move(ready)]() mutable {
        HANDLE hThread = GetCurrentThread();
        SetThreadPriority(hThread, THREAD_MODE_BACKGROUND_BEGIN);
        
        THREAD_POWER_THROTTLING_STATE pts = {};
        pts.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
        pts.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        pts.StateMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        SetThreadInformation(hThread, ThreadPowerThrottling, &pts, sizeof(pts));

        dispatcher = Dispatcher::GetCurrent();
        dispatcher->InvokeAsync([&] {
            p.set_value();
        });
        dispatcher->Run(false);
    });

    readyFuture.get();
}

WorkerThread::~WorkerThread()
{
    if(dispatcher)
        dispatcher->Quit();

    if(worker.joinable())
        worker.join();
}

WorkerThread& WorkerThread::GetInstance()
{
    static WorkerThread instance;
    return instance;
}

} // fraze

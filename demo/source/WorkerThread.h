#pragma once
#include <fraze/program/Dispatcher.h>
#include <thread>

namespace fraze
{

class WorkerThread
{
    std::thread worker;
    sptr<Dispatcher> dispatcher;
public:
    WorkerThread();
    ~WorkerThread();

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    template<typename Func>
    void InvokeAsync(Func&& func)
    {
        dispatcher->InvokeAsync(std::forward<Func>(func));
    }

    static WorkerThread& GetInstance();
};

} // fraze

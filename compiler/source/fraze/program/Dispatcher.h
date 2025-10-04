/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Pointers.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace fraze {

using steady_clock = std::chrono::steady_clock;

struct DispatchAction
{
    std::function<void()> function;
    steady_clock::time_point when{};

    DispatchAction(){}

    template<class Func>
    DispatchAction(
        Func&& function,
        steady_clock::time_point when = {}
    ) : function(std::forward<Func>(function)), when(when) {}
};

class Dispatcher : public sptr_from_this<Dispatcher>
{
    mutable std::mutex mut;
    mutable std::condition_variable cv;
    std::atomic<bool> run = false;
    std::vector<sptr<DispatchAction>> actions;
    static thread_local sptr<Dispatcher> _currentDispatcher;
public:
    virtual sptr<DispatchAction> InvokeAsync(
        std::function<void()> function,
        steady_clock::time_point when = steady_clock::time_point{ steady_clock::duration::zero() }
    );
    virtual bool Cancel(const sptr<DispatchAction>& action);
    static sptr<Dispatcher> GetCurrent();
    virtual void Run(bool quitWhenDone = false);
    virtual void Quit();
};

} // fraze

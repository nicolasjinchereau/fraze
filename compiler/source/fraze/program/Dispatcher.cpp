/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/Dispatcher.h>
#include <fraze/common/Exception.h>
#include <print>

namespace fraze {

thread_local sptr<Dispatcher> Dispatcher::_currentDispatcher;

struct DispatchActionComparison
{
    bool operator()(const sptr<DispatchAction>& left, const sptr<DispatchAction>& right) const {
        return left->when < right->when;
    }
};

// Queues an action in deadline order, ties in arrival order, so a run-now post can't overtake an action already due.
sptr<DispatchAction> Dispatcher::InvokeAsync(
    std::function<void()> function, steady_clock::time_point when)
{
    std::unique_lock<std::mutex> lk(mut);

    auto action = spnew<DispatchAction>(std::move(function), when);
    auto it = std::upper_bound(actions.begin(), actions.end(), action, DispatchActionComparison());
    actions.insert(it, std::move(action));
    cv.notify_one();

    return action;
}

bool Dispatcher::Cancel(const sptr<DispatchAction>& action)
{
    std::unique_lock<std::mutex> lk(mut);

    auto it = std::find(actions.begin(), actions.end(), action);
    if (it != actions.end()) {
        actions.erase(it);
        return true;
    }

    return false;
}

void Dispatcher::Run(bool quitWhenDone)
{
    run = true;

    while(run)
    {
        sptr<DispatchAction> action;

        {
            std::unique_lock<std::mutex> lk(mut);

            if(!(actions.empty() && quitWhenDone))
            {
                auto wake = !actions.empty()
                    ? actions.front()->when
                    : steady_clock::time_point::max();

                cv.wait_until(
                    lk, wake,
                    [this]{ return !run || (!actions.empty() && steady_clock::now() >= actions.front()->when); }
                );
            }

            if(run && !actions.empty())
            {
                action = std::move(actions.front());
                actions.erase(actions.begin());
            }
        }

        if(action)
            action->function();
        else
            run = false;
    }
}

void Dispatcher::Quit()
{
    std::unique_lock<std::mutex> lk(mut);
    run = false;
    actions.clear();
    cv.notify_one();
}

sptr<Dispatcher> Dispatcher::GetCurrent()
{
    if(!_currentDispatcher)
        _currentDispatcher = spnew<Dispatcher>();

    return _currentDispatcher;
}

} // fraze

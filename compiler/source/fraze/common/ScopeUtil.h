/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <utility>

namespace fraze {

template<typename EF>
class scope_exit
{
    EF func;
    bool active;
public:
    explicit scope_exit(EF&& func) noexcept
        : func(std::forward<EF>(func)), active(true) {}

    scope_exit(scope_exit&& other) noexcept
        : func(std::move(other.func)), active(other.active)
    {
        other.active = false;
    }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;

    ~scope_exit()
    {
        if (active)
            func();
    }

    void release() noexcept {
        active = false;
    }
};

} // fraze

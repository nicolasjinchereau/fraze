/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <exception>
#include <utility>
#include <fraze/common/jump/JumpPoint.h>

namespace fraze {

// Where execution resumes when an exception has to cross frames it can't unwind, such as the JIT's. Every
// native -> compiled edge opens one and every compiled -> native edge jumps to the innermost, carrying the
// exception in it. They nest, because an extern can call back into compiled code, so each pad remembers the
// one it displaced.
class LandingPad
{
    std::exception_ptr failure;
    LandingPad* enclosing;

    static inline thread_local LandingPad* innermost = nullptr;

public:
    // SaveJumpPoint has to be called by the function that owns the pad, because it captures that function's
    // stack and return address, so the pad exposes the point instead of saving it.
    JumpPoint jumpPoint{};

    LandingPad() : enclosing(std::exchange(innermost, this)) {}
    ~LandingPad() { innermost = enclosing; }

    LandingPad(const LandingPad&) = delete;
    LandingPad& operator=(const LandingPad&) = delete;

    static LandingPad* GetInnermost() { return innermost; }

    // Stores what a stub caught, to be rethrown where the pad was saved. Called before leaving the 'catch',
    // and the jump after it.
    void StoreFailure(std::exception_ptr exception) { failure = std::move(exception); }

    [[noreturn]] void JumpToFailureHandling() { JumpTo(jumpPoint, 1); }

    // Rethrows in the frame that saved the pad, which native code can unwind out of normally.
    [[noreturn]] void RethrowFailure()
    {
        std::exception_ptr exception = std::move(failure);
        std::rethrow_exception(exception);
    }
};

} // fraze

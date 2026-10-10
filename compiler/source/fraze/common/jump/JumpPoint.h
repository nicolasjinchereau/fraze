/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstddef>
#include <cstdint>

namespace fraze {

// A place on this thread's stack to resume at, and the register state to resume with. Unlike the CRT's
// setjmp/longjmp, a jump to it never unwinds: no destructor runs and no exception handler is entered on the
// way, which is what lets a jump cross a frame that has no unwind data, such as one of the JIT's.
//
// JumpPoint.x64.asm reads these fields by offset, so the static asserts below are part of its interface.
struct alignas(16) JumpPoint
{
    std::byte xmm[160]; // xmm6-xmm15
    uint32_t mxcsr;
    uint16_t x87ControlWord;
    uint16_t reserved;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rsp;
    uint64_t rip;
};

static_assert(alignof(JumpPoint) == 16, "movaps needs the xmm storage 16-byte aligned");
static_assert(offsetof(JumpPoint, xmm) == 0x00);
static_assert(offsetof(JumpPoint, mxcsr) == 0xA0);
static_assert(offsetof(JumpPoint, x87ControlWord) == 0xA4);
static_assert(offsetof(JumpPoint, rbx) == 0xA8);
static_assert(offsetof(JumpPoint, rbp) == 0xB0);
static_assert(offsetof(JumpPoint, rdi) == 0xB8);
static_assert(offsetof(JumpPoint, rsi) == 0xC0);
static_assert(offsetof(JumpPoint, r12) == 0xC8);
static_assert(offsetof(JumpPoint, r13) == 0xD0);
static_assert(offsetof(JumpPoint, r14) == 0xD8);
static_assert(offsetof(JumpPoint, r15) == 0xE0);
static_assert(offsetof(JumpPoint, rsp) == 0xE8);
static_assert(offsetof(JumpPoint, rip) == 0xF0);
static_assert(sizeof(JumpPoint) == 0xF8 + 8);

extern "C" {

// Returns 0 having filled 'point' in, and returns again with the value JumpTo was given once something jumps
// to it. The frames above the call must still be live then, so a function that saves a point must not return
// before the jumps to it are done.
int SaveJumpPoint(JumpPoint& point);

// Resumes where SaveJumpPoint returned. 'value' is what it returns there, and 0 would make the landing
// indistinguishable from the save.
[[noreturn]] void JumpTo(JumpPoint& point, int value);

} // extern "C"

} // fraze

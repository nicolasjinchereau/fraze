#/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <span>
#include <vector>
#include <fraze/memory/Allocator.h>

namespace fraze {

class Program;
class Object;
struct TypeInfo;

class ScopedAllocator : public Allocator
{
public:
    std::vector<std::byte*> allocated;

    ScopedAllocator(Program* program);
    ~ScopedAllocator();

    ScopedAllocator(const ScopedAllocator&) = delete;
    ScopedAllocator& operator=(const ScopedAllocator&) = delete;

    ScopedAllocator(ScopedAllocator&& other) noexcept
        : Allocator(other._program)
        , allocated(std::move(other.allocated))
    {
        other._program = nullptr;
    }

    ScopedAllocator& operator=(ScopedAllocator&& other) noexcept
    {
        if (this != &other)
        {
            allocated = std::move(other.allocated);
            _program = other._program;
            other._program = nullptr;
        }
        return *this;
    }

    virtual std::byte* Allocate(size_t size) override;
};

} // fraze

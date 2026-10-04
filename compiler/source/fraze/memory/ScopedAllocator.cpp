#/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/ScopedAllocator.h>
#include <fraze/program/Program.h>

namespace fraze {

ScopedAllocator::ScopedAllocator(Program* program)
    : Allocator(program)
{
}

ScopedAllocator::~ScopedAllocator()
{
    program()->UnpinMemory(allocated);
}

std::byte* ScopedAllocator::Allocate(size_t size)
{
    std::byte* p = program()->heap.Allocate(size, true);
    allocated.push_back(p);
    return p;
}

} // fraze

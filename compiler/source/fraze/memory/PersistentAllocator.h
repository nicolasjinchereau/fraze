/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/memory/Allocator.h>

namespace fraze {

class Program;

// Allocates from the heap's persistent pages, which a collection neither traces nor frees.
class PersistentAllocator : public Allocator
{
public:
    PersistentAllocator(Program* program) : Allocator(program) {}
    virtual std::byte* Allocate(size_t size) override;
};

} // fraze

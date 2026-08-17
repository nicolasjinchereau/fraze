#/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/memory/IAllocator.h>

namespace fraze {

class Program;

class DefaultAllocator : public IAllocator
{
public:
    DefaultAllocator(Program* program) : IAllocator(program) {}
    virtual std::byte* Allocate(size_t size) override;
};

} // fraze

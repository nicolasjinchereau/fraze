#/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/memory/DefaultAllocator.h>
#include <fraze/program/Program.h>

namespace fraze {

std::byte* DefaultAllocator::Allocate(size_t size)
{
    return program()->heap.Allocate(size, false);
}

} // fraze

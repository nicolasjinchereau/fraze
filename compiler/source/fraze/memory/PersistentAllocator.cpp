/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/PersistentAllocator.h>
#include <fraze/program/Program.h>

namespace fraze {

std::byte* PersistentAllocator::Allocate(size_t size)
{
    return program()->heap.AllocatePersistent(size);
}

} // fraze

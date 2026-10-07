/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/MIRContext.h>
#include <fraze/common/Exception.h>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
#include <mir.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fraze {

SourceLocation MIRContext::errorLocation;

[[noreturn]] static void ThrowMIRError(MIR_error_type_t errorType, const char* format, ...)
{
    char message[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    Throw(MIRContext::errorLocation, "MIR error: {}", message);
    std::unreachable();
}

MIRContext::MIRContext()
{
    context = MIR_init2(&allocator, &codeAllocator);
    MIR_set_error_func(context, ThrowMIRError);
}

MIRContext::~MIRContext()
{
    context = nullptr;
    FreeAllBlocks();
    ReleaseCodePages();
}

void* MIRContext::Malloc(std::size_t size, void* userData)
{
    MIRContext& self = *static_cast<MIRContext*>(userData);

    void* storage = std::malloc(sizeof(Block) + size);

    if(!storage)
        return nullptr;

    // link() requires an unlinked node, and the storage holds whatever malloc left in it.
    Block* block = new (storage) Block{};
    self.blocks.push_back(block);

    return PayloadOf(block);
}

void* MIRContext::Calloc(std::size_t count, std::size_t size, void* userData)
{
    if(size != 0 && count > std::size_t(-1) / size)
        return nullptr;

    std::size_t total = count * size;
    void* payload = Malloc(total, userData);
    if(payload)
        std::memset(payload, 0, total);

    return payload;
}

// oldSize is for allocators that have to copy the payload themselves; std::realloc does it.
void* MIRContext::Realloc(void* payload, std::size_t oldSize, std::size_t newSize, void* userData)
{
    if(!payload)
        return Malloc(newSize, userData);

    MIRContext& self = *static_cast<MIRContext*>(userData);

    Block* moved = static_cast<Block*>(std::realloc(BlockOf(payload), sizeof(Block) + newSize));

    if(!moved)
        return nullptr;

    self.blocks.relink_neighbors(moved);

    return PayloadOf(moved);
}

void MIRContext::Free(void* payload, void* userData)
{
    if(!payload)
        return;

    MIRContext& self = *static_cast<MIRContext*>(userData);

    Block* block = BlockOf(payload);
    self.blocks.remove(block);
    block->~Block();
    std::free(block);
}

void* MIRContext::MemMap(std::size_t length, void* userData)
{
    MIRContext& self = *static_cast<MIRContext*>(userData);

    void* pages = VirtualAlloc(nullptr, length, MEM_COMMIT, PAGE_EXECUTE);

    if(pages)
        self.codePages.push_back(pages);

    return pages;
}

// MEM_RELEASE takes the base address and a zero length, which is why MIR's own default_mem_unmap fails with
// ERROR_INVALID_PARAMETER and never releases a page.
int MIRContext::MemUnmap(void* pages, std::size_t length, void* userData)
{
    MIRContext& self = *static_cast<MIRContext*>(userData);

    auto it = std::find(self.codePages.begin(), self.codePages.end(), pages);

    if(it == self.codePages.end())
        return -1;

    self.codePages.erase(it);

    return VirtualFree(pages, 0, MEM_RELEASE) ? 0 : -1;
}

int MIRContext::MemProtect(void* pages, std::size_t length, MIR_mem_protect_t protection, void* userData)
{
    DWORD pageProtection = protection == PROT_WRITE_EXEC ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    DWORD previousProtection = 0;

    return VirtualProtect(pages, length, pageProtection, &previousProtection) ? 0 : -1;
}

void MIRContext::FreeAllBlocks()
{
    while(!blocks.empty())
    {
        Block* block = &blocks.front();
        blocks.pop_front();
        block->~Block();
        std::free(block);
    }
}

void MIRContext::ReleaseCodePages()
{
    for(void* pages : codePages)
    {
        VirtualFree(pages, 0, MEM_RELEASE);
    }

    codePages.clear();
}

} // fraze

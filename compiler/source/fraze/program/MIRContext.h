/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstddef>
#include <vector>
#include <fraze/common/IntrusiveList.h>
#include <fraze/common/SourceLocation.h>
#include <mir-alloc.h>
#include <mir-code-alloc.h>

struct MIR_context;

namespace fraze {

class MIRContext
{
    struct Block : intrusive_list_node
    {
    };

    static_assert(sizeof(Block) % alignof(std::max_align_t) == 0,
        "Block must be a whole number of max_align_t, so the payload after it keeps malloc's alignment");

    intrusive_list<Block> blocks;
    std::vector<void*> codePages;

    static Block* BlockOf(void* payload) {
        return static_cast<Block*>(payload) - 1;
    }

    static void* PayloadOf(Block* block) {
        return block + 1;
    }

    static void* Malloc(std::size_t size, void* userData);
    static void* Calloc(std::size_t count, std::size_t size, void* userData);
    static void* Realloc(void* payload, std::size_t oldSize, std::size_t newSize, void* userData);
    static void Free(void* payload, void* userData);

    static void* MemMap(std::size_t length, void* userData);
    static int MemUnmap(void* pages, std::size_t length, void* userData);
    static int MemProtect(void* pages, std::size_t length, MIR_mem_protect_t protection, void* userData);

    // Custom allocators eliminate the need to call MIR_finish, which may not work properly after
    // an error occurs, especially if we throw a fraze::Exception from the handler.
    MIR_alloc allocator{ &Malloc, &Calloc, &Realloc, &Free, this };
    MIR_code_alloc codeAllocator{ &MemMap, &MemUnmap, &MemProtect, this };
    MIR_context* context{};

    void FreeAllBlocks();
    void ReleaseCodePages();

public:
    MIRContext();
    ~MIRContext();

    MIRContext(const MIRContext&) = delete;
    MIRContext(MIRContext&&) = delete;
    MIRContext& operator=(const MIRContext&) = delete;
    MIRContext& operator=(MIRContext&&) = delete;

    // An error location maintained by JITCodeGenerator::VisitChildNode, to be used by ThrowMIRError.
    static SourceLocation errorLocation;

    operator MIR_context*() const {
        return context;
    }
};

} // fraze

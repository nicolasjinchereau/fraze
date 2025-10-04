/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/Heap.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <print>

namespace fraze {

struct HeapObject
{
    size_t size;

#if FRAZE_HEAP_DEBUG
    SourceLocation* pLocation;
#endif

    std::byte* payload() {
        return reinterpret_cast<std::byte*>(this) + sizeof(HeapObject);
    }

    size_t payloadSize() const {
        return size - sizeof(HeapObject);
    }

    static HeapObject* from(std::byte* pPayload) {
        return reinterpret_cast<HeapObject*>(pPayload - sizeof(HeapObject));
    }
};

Page::Page(size_t requestedSize)
{
    totalSize = (std::max(requestedSize, MinPageSize) + BlockSize - 1) & ~( BlockSize - 1 );
    blockCount = totalSize / BlockSize;
    totalUsed = 0;
    nextFree = 0;
    storage = std::unique_ptr<std::byte[]>( new std::byte[totalSize]{} );
    starts.resize(blockCount);
    inuse.resize(blockCount);
    marked.resize(blockCount);
}

std::byte* Page::Allocate(size_t size)
{
    size_t blocksNeeded = (size + BlockSize - 1) / BlockSize;
    if(blocksNeeded > blockCount)
        return nullptr;

    size_t i = nextFree;
    size_t wrapped = 0;

    while(wrapped < 2)
    {
        size_t scanEnd = (wrapped == 0) ? blockCount - blocksNeeded + 1 : nextFree;

        while(i < scanEnd)
        {
            size_t j = 0;

            for( ; j != blocksNeeded; ++j)
            {
                if(inuse[i + j])
                    break;
            }

            if(j == blocksNeeded)
            {
                starts[i] = true;

                for(size_t k = 0; k != blocksNeeded; ++k)
                    inuse[i + k] = true;

                totalUsed += blocksNeeded * BlockSize;

                nextFree = i + blocksNeeded;
                if(nextFree >= blockCount)
                    nextFree = 0;

                return storage.get() + i * BlockSize;
            }

            i += j + 1;
        }

        i = 0;
        ++wrapped;
    }

    return nullptr;
}

void Page::Deallocate(const std::byte* p)
{
    assert(p);

    size_t block = static_cast<size_t>(p - storage.get()) / BlockSize;
    assert(block < blockCount);
    assert(starts[block]);

    starts[block] = false;

    size_t nextStart = block + 1;

    for( ; nextStart != blockCount; ++nextStart)
    {
        if(starts[nextStart])
            break;
    }

    while(block != nextStart && inuse[block])
    {
        auto p = storage.get() + block * BlockSize;
        std::fill(p, p + BlockSize, std::byte(0));
        inuse[block] = false;
        ++block;
        totalUsed -= BlockSize;
    }
}

bool Page::Contains(const std::byte* p) const
{
    auto* begin = storage.get();
    auto* end = begin + totalSize;
    return p >= begin && p < end;
}

size_t Page::GetHeapObjectStart(const std::byte* p) const
{
    assert(p);

    if(!Contains(p))
        return InvalidIndex;
    
    size_t i = static_cast<size_t>(p - storage.get()) / BlockSize;

    while(inuse[i] && !starts[i] && i > 0)
        --i;

    if(!starts[i] || !inuse[i])
        return InvalidIndex;

    return i;
}

HeapObject* Page::GetHeapObject(size_t blockIndex)
{
    assert(blockIndex < blockCount);
    assert(starts[blockIndex]);
    return reinterpret_cast<HeapObject*>(storage.get() + blockIndex * BlockSize);
}

// HEAP

std::byte* Heap::Allocate(size_t size)
{
    std::lock_guard<std::mutex> lk(mut);

    size_t totalSize = sizeof(HeapObject) + size;

    if(pages.empty())
    {
        pages.emplace_back(totalSize);
    }

    auto p = AllocateFromExistingPages(totalSize);
    if(!p)
    {
        CollectInternal();
        p = AllocateFromExistingPages(totalSize);
    }

    if(!p)
    {
        pages.emplace_back(totalSize);
        p = pages.back().Allocate(totalSize);
    }

    HeapObject* pBlock = reinterpret_cast<HeapObject*>(p);
    pBlock->size = totalSize;

#if FRAZE_HEAP_DEBUG
    pBlock->pLocation = pLocation;
    LogLocation("Alloc", pLocation);
#endif

    return pBlock->payload();
}

void Heap::Collect()
{
    std::lock_guard<std::mutex> lk(mut);
    CollectInternal();
}

void Heap::Report()
{
    std::lock_guard<std::mutex> lk(mut);

    size_t totalSpace = 0;
    size_t totalUsed = 0;

    for(auto& page : pages)
    {
        totalSpace += page.totalSize;
        totalUsed += page.totalUsed;
    }

    std::cout << "Usage: " << totalUsed << "/" << totalSpace << std::endl;
}

void Heap::AddRange(std::span<std::byte> range)
{
    std::lock_guard<std::mutex> lk(mut);
    ranges.push_back(range);
}

void Heap::RemoveRange(std::byte* rangeStart)
{
    std::lock_guard<std::mutex> lk(mut);

    for(auto it = ranges.begin(); it != ranges.end(); ++it)
    {
        if(it->data() == rangeStart)
        {
            ranges.erase(it);
            break;
        }
    }
}

void Heap::PinMemory(const std::byte* p) {
    std::lock_guard<std::mutex> lk(mut);
    pinned.insert(p);
}

void Heap::UnpinMemory(const std::byte* p) {
    std::lock_guard<std::mutex> lk(mut);
    pinned.erase(p);
}

void Heap::SetStack(stack<Word>* pStack) {
    std::lock_guard<std::mutex> lk(mut);
    this->pStack = pStack;
}

#if FRAZE_HEAP_DEBUG
void Heap::SetLocation(SourceLocation* pLoc) {
    std::lock_guard<std::mutex> lk(mut);
    pLocation = pLoc;
}
#endif

std::byte* Heap::AllocateFromExistingPages(size_t size)
{
    for(auto& page : pages)
    {
        auto p = page.Allocate(size);
        if(p)
            return p;
    }

    return nullptr;
}

void Heap::CollectInternal()
{
#if FRAZE_HEAP_DEBUG
    LogLocation("Collect", pLocation);
#endif

    // globals are at the bottom of the stack
    if(pStack && !pStack->empty())
    {
        std::byte* stackBegin = reinterpret_cast<std::byte*>(pStack->begin());
        std::byte* stackEnd = reinterpret_cast<std::byte*>(pStack->end());
        ScanRange({ stackBegin, stackEnd });
    }

    for(auto& range : ranges)
    {
        ScanRange(range);
    }

    for(auto& p : pinned)
    {
        Mark(p);
    }

    size_t deallocations = 0;

    for(auto& page : pages)
    {
        for(size_t i = 0; i != page.blockCount; ++i)
        {
            if(page.marked[i])
            {
                page.marked[i] = false;
            }
            else if(page.starts[i])
            {
                HeapObject* pObject = page.GetHeapObject(i);

#if FRAZE_HEAP_DEBUG
                LogLocation("Free", pObject->pLocation);
#endif

                page.Deallocate(reinterpret_cast<std::byte*>(pObject));
                ++deallocations;
            }
        }
    }

    //std::cout << "Collected: " << deallocations << std::endl;
}

void Heap::ScanRange(std::span<std::byte> range)
{
    assert(range.data());
    assert(!range.empty());
    
    std::byte* start = range.data();
    std::byte* end = start + range.size();

    for(auto p = start; p < end; p += sizeof(Word))
    {
        std::byte* potentialPointer = *reinterpret_cast<std::byte**>(p);
        Mark(potentialPointer);
    }
}

void Heap::Mark(const std::byte* p)
{
    if(!p)
        return;

    for(auto& page : pages)
    {
        size_t startBlockIndex = page.GetHeapObjectStart(p);
        if(startBlockIndex != Page::InvalidIndex)
        {
            HeapObject* pObject = page.GetHeapObject(startBlockIndex);

#if FRAZE_HEAP_DEBUG
            LogLocation("Mark", pObject->pLocation);
#endif

            if(page.marked[startBlockIndex])
                return;

            for(auto i = 0; i < pObject->size; i += Page::BlockSize)
            {
                page.marked[startBlockIndex + i / Page::BlockSize] = true;
            }

            for(size_t i = 0; i < pObject->payloadSize(); i += sizeof(Word))
            {
                auto potentialPointer = *reinterpret_cast<std::byte**>(pObject->payload() + i);
                Mark(potentialPointer);
            }

            break;
        }
    }
}

#if FRAZE_HEAP_DEBUG
void Heap::LogLocation(const char* tag, const SourceLocation* pLoc)
{
    if(!logFile.is_open())
        return;

    std::string data = std::format("{}:\n  {}({})\n  {}\n",
        tag,
        pLoc ? pLoc->file : std::string_view("<unknown file>"),
        pLoc ? pLoc->line : std::size_t(0),
        pLoc ? trim_left(pLoc->lineText) : std::string_view("<no text>"));

    logFile << data;
    std::print("{}", data);
}
#endif

} // fraze
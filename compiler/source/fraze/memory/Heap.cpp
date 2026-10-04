/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/Heap.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <fraze/program/Program.h>
#include <print>

namespace fraze {

Page::Page(size_t requestedSize)
{
    totalSize = (std::max(requestedSize, Heap::MinPageSize) + Heap::BlockSize - 1) & ~(Heap::BlockSize - 1 );
    blockCount = totalSize / Heap::BlockSize;
    totalUsed = 0;
    nextFree = 0;
    freeRunLimit = blockCount;

    void* mem = ::operator new(totalSize, std::align_val_t(Heap::BlockSize));
    storage.reset(static_cast<std::byte*>(mem));

    starts.resize(blockCount, false);
    inuse.resize(blockCount, false);
    color.resize(blockCount, Heap::InitialBlockColor);

#if FRAZE_HEAP_DEBUG
    locations.resize(blockCount, nullptr);
#endif
}

std::byte* Page::Allocate(size_t size)
{
    assert(size != 0);
    assert(freeRunLimit <= blockCount);

    size_t blocksNeeded = (size + Heap::BlockSize - 1) / Heap::BlockSize;
    if(blocksNeeded > freeRunLimit)
        return nullptr;

    const size_t startLimit = blockCount - blocksNeeded + 1;

    size_t i = nextFree;
    size_t wrapped = 0;

    while(wrapped < 2)
    {
        size_t scanEnd = (wrapped == 0) ? startLimit : std::min(nextFree, startLimit);

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
                {
                    inuse[i + k] = true;
                    color[i + k] = Heap::InitialBlockColor;
                }

                totalUsed += blocksNeeded * Heap::BlockSize;

                nextFree = i + blocksNeeded;
                if(nextFree >= blockCount)
                    nextFree = 0;

                std::byte* p = storage.get() + i * Heap::BlockSize;

                // Heap::Mark scans whole blocks, so the slack past size must not hold stale pointers.
                std::fill(p + size, p + blocksNeeded * Heap::BlockSize, std::byte(0));

                return p;
            }

            i += j + 1;
        }

        i = 0;
        ++wrapped;
    }

    // Both passes together cover every start position, so no run this long exists.
    freeRunLimit = blocksNeeded - 1;

    return nullptr;
}

void Page::Deallocate(const std::byte* p)
{
    assert(p);

    size_t block = static_cast<size_t>(p - storage.get()) / Heap::BlockSize;
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
#if FRAZE_HEAP_DEBUG
        auto pByte = storage.get() + block * Heap::BlockSize;
        std::fill(pByte, pByte + Heap::BlockSize, std::byte(0));
#endif
        inuse[block] = false;
        ++block;
        totalUsed -= Heap::BlockSize;
    }

    freeRunLimit = blockCount;
}

bool Page::Contains(const std::byte* p) const
{
    auto* begin = storage.get();
    auto* end = begin + totalSize;
    return p >= begin && p < end;
}

size_t Page::GetAllocationStart(const std::byte* p) const
{
    assert(p);
    assert(Contains(p));
    
    size_t i = static_cast<size_t>(p - storage.get()) / Heap::BlockSize;

    while(inuse[i] && !starts[i] && i > 0)
        --i;

    if(!starts[i] || !inuse[i])
        return InvalidIndex;

    return i;
}

std::byte* Page::GetBlock(size_t blockIndex)
{
    assert(blockIndex < blockCount);
    assert(starts[blockIndex]);
    return storage.get() + blockIndex * Heap::BlockSize;
}

// HEAP

Heap::Heap(Program* pProgram)
    : pProgram(pProgram)
{
    assert(pProgram);

    pages.reserve(32);
    sortedPageInfo.reserve(32);

#if FRAZE_HEAP_DEBUG
    std::filesystem::path outputPath = "output";

    std::error_code ec;
    std::filesystem::create_directories(outputPath, ec);
    assert(!ec && "Failed to create output path for Heap log");

    outputPath /= "heap-log.txt";
    outputPath.make_preferred();

    logFile.open(outputPath, std::ios::out);
#endif
}

std::byte* Heap::AllocateRaw(size_t size)
{
    std::byte* p = nullptr;

    if(size > Heap::MinPageSize / 2 || pages.empty())
    {
        auto page = AddPage(size);
        return page->Allocate(size);
    }

    if(auto p = AllocateFromExistingPages(size))
    {
        return p;
    }

    CollectInternal();
    
    if(auto p = AllocateFromExistingPages(size))
    {
        return p;
    }

    size_t nextPageSize = std::max(TotalUsed() / 2, MinGrownPageSize);
    auto page = AddPage(nextPageSize);
    return page->Allocate(size);
}

std::byte* Heap::Allocate(size_t size, bool pin)
{
    std::lock_guard<std::mutex> lk(mut);

    if(auto block = AllocateRaw(size))
    {
#if FRAZE_HEAP_DEBUG
        Page* page = FindPage(block);
        page->locations[page->GetAllocationStart(block)] = pLocation;
        LogLocation("Alloc", pLocation);
#endif
        if(pin)
            pinned.insert(block);

        return block;
    }

    return nullptr;
}

std::byte* Heap::AllocatePersistent(size_t size)
{
    std::lock_guard<std::mutex> lk(mut);

    for(auto& page : persistentPages)
    {
        if(auto p = page.Allocate(size))
            return p;
    }

    return persistentPages.emplace_back(size).Allocate(size);
}

void Heap::DeallocatePersistent(const std::byte* p)
{
    std::lock_guard<std::mutex> lk(mut);

    auto it = std::find_if(
        persistentPages.begin(), persistentPages.end(),
        [&](const Page& page) {
            return page.Contains(p);
        });

    assert(it != persistentPages.end());
    it->Deallocate(p);
}

Page* Heap::AddPage(size_t requestedSize)
{
    pages.emplace_back(requestedSize);

    Page* page = &pages.back();
    std::byte* begin = page->storage.get();
    std::byte* end = begin + page->totalSize;

    auto it = std::lower_bound(
        sortedPageInfo.begin(), sortedPageInfo.end(), begin,
        [](const PageInfo& info, const std::byte* value) {
            return info.begin < value;
        });

    sortedPageInfo.insert(it, PageInfo{ begin, end, pages.size() - 1 });

    minAddress = sortedPageInfo.front().begin;
    maxAddress = sortedPageInfo.back().end;

    return page;
}

void Heap::RemovePage(size_t pageIndex)
{
    auto it = std::find_if(
        sortedPageInfo.begin(), sortedPageInfo.end(),
        [&](const PageInfo& r) {
            return r.pageIndex == pageIndex;
        });

    if(it == sortedPageInfo.end())
        return;

    pages.erase(pages.begin() + pageIndex);
    sortedPageInfo.erase(it);

    for(auto& info : sortedPageInfo)
    {
        if(info.pageIndex > pageIndex)
        {
            --info.pageIndex;
        }
    }

    minAddress = !sortedPageInfo.empty() ? sortedPageInfo.front().begin : nullptr;
    maxAddress = !sortedPageInfo.empty() ? sortedPageInfo.back().end : nullptr;
}

Page* Heap::FindPage(const std::byte* p)
{
    if(!p || sortedPageInfo.empty() || p < minAddress || p >= maxAddress)
        return nullptr;

    auto it = std::lower_bound(
        sortedPageInfo.begin(), sortedPageInfo.end(), p,
        [](const PageInfo& info, const std::byte* p) {
            return info.end <= p;
        });

    if(it == sortedPageInfo.end() || p < it->begin)
        return nullptr;

    return &pages[it->pageIndex];
}

void Heap::Collect()
{
    std::lock_guard<std::mutex> lk(mut);
    CollectInternal();
}

size_t Heap::TotalUsed() const
{
    size_t used = 0;
    
    for(auto& page : pages)
        used += page.totalUsed;

    return used;
}

void Heap::Report()
{
    std::lock_guard<std::mutex> lk(mut);

    size_t totalUsed = 0;
    size_t totalFree = 0;

    for(auto& page : pages)
    {
        totalUsed += page.totalUsed;
        totalFree += page.totalSize - page.totalUsed;
    }

    std::locale::global(std::locale("en_US.UTF-8"));
    std::println("used memory: {:L} bytes", totalUsed);
    std::println("free memory: {:L} bytes", totalFree);
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

void Heap::UnpinMemory(const std::span<std::byte*> ps)
{
    std::lock_guard<std::mutex> lk(mut);

    for(auto& p : ps)
        pinned.erase(p);
}

#if FRAZE_HEAP_DEBUG
void Heap::SetLocation(const std::source_location& nativeLocation)
{
    std::lock_guard<std::mutex> lk(mut);
    auto key = std::tuple(std::string_view(nativeLocation.file_name()), nativeLocation.line(), nativeLocation.column());
    pLocation = &nativeLocations.try_emplace(key, nativeLocation).first->second;
}
#endif

std::byte* Heap::AllocateFromExistingPages(size_t size)
{
    for(size_t i = 0; i != pages.size(); ++i)
    {
        size_t pageIndex = (nextPage + i) % pages.size();

        if(auto p = pages[pageIndex].Allocate(size))
        {
            nextPage = pageIndex;
            return p;
        }
    }

    return nullptr;
}

void Heap::CollectInternal()
{
#if FRAZE_HEAP_DEBUG
    LogLocation("Collect", pLocation);
#endif

    ++currentColor;

    if(currentColor == Heap::InitialBlockColor)
        ++currentColor;

    auto& globals = pProgram->globals;
    if(!globals.empty())
        ScanRange(std::as_writable_bytes(std::span(globals.data(), globals.size())));

    std::span<Word> usedStackRange = pProgram->GetUsedStackRange();
    if(!usedStackRange.empty())
        ScanRange(std::as_writable_bytes(usedStackRange));

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
            if(page.starts[i] && page.color[i] != currentColor)
            {
#if FRAZE_HEAP_DEBUG
                LogLocation("Free", page.locations[i]);
#endif

                page.Deallocate(page.GetBlock(i));
                ++deallocations;
            }
        }
    }
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
    if((reinterpret_cast<uintptr_t>(p) & (alignof(Object*) - 1)) != 0)
        return;

    Page* page = FindPage(p);
    if(!page)
        return;

    size_t startBlockIndex = page->GetAllocationStart(p);
    if(startBlockIndex == Page::InvalidIndex)
        return;

#if FRAZE_HEAP_DEBUG
    LogLocation("Mark", page->locations[startBlockIndex]);
#endif

    if(page->color[startBlockIndex] == currentColor)
        return;

    page->color[startBlockIndex] = currentColor;

    size_t endBlockIndex = startBlockIndex + 1;

    for( ; endBlockIndex != page->blockCount && page->inuse[endBlockIndex] && !page->starts[endBlockIndex]; ++endBlockIndex)
    {
        page->color[endBlockIndex] = currentColor;
    }

    std::byte* pObject = page->GetBlock(startBlockIndex);
    size_t allocatedSize = (endBlockIndex - startBlockIndex) * Heap::BlockSize;

    for(size_t i = 0; i < allocatedSize; i += sizeof(Word))
    {
        auto potentialPointer = *reinterpret_cast<std::byte**>(pObject + i);
        Mark(potentialPointer);
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
        pLoc ? trim_left(pLoc->GetLineText()) : std::string_view("<no text>"));

    logFile << data;
    std::print("{}", data);
}
#endif

} // fraze
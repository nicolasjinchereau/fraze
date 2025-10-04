/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <fstream>
#include <span>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <fraze/common/SourceLocation.h>
#include <fraze/common/Stack.h>

namespace fraze {

class Word;
struct HeapObject;

struct Page
{
    size_t totalSize{};
    size_t blockCount{};
    size_t totalUsed{};
    size_t nextFree{};
    std::unique_ptr<std::byte[]> storage;
    std::vector<uint8_t> starts;
    std::vector<uint8_t> inuse;
    std::vector<uint8_t> marked;
    static constexpr size_t MinPageSize = 8192;
    static constexpr size_t BlockSize = 16U;
    static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

    Page(size_t requestedSize = MinPageSize);

    std::byte* Allocate(size_t size);
    void Deallocate(const std::byte* p);
    bool Contains(const std::byte* p) const;
    size_t GetHeapObjectStart(const std::byte* p) const;
    HeapObject* GetHeapObject(size_t blockIndex);
};

class Heap
{
#if FRAZE_HEAP_DEBUG
    SourceLocation* pLocation{};
    std::ofstream logFile;
#endif

    std::vector<Page> pages;
    std::vector<std::span<std::byte>> ranges;
    stack<Word>* pStack{};
    std::unordered_set<const std::byte*> pinned;
    std::mutex mut;

public:
    Heap()
    {
#if FRAZE_HEAP_DEBUG
        logFile.open("heap-log.txt", std::ios::out);
#endif
    }

    std::byte* Allocate(size_t size);
    void Collect();
    void Report();
    void AddRange(std::span<std::byte> range);
    void RemoveRange(std::byte* rangeStart);
    void PinMemory(const std::byte* p);
    void UnpinMemory(const std::byte* p);
    void SetStack(stack<Word>* pStack);

#if FRAZE_HEAP_DEBUG
    void SetLocation(SourceLocation* pLocation);
#endif

private:
    std::byte* AllocateFromExistingPages(size_t size);
    void CollectInternal();
    void ScanRange(std::span<std::byte> range);
    void Mark(const std::byte* p);

#if FRAZE_HEAP_DEBUG
    void LogLocation(const char* tag, const SourceLocation* pLoc);
#endif
};

} // fraze

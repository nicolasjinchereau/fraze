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
struct Page;
class Program;

struct PageInfo
{
    std::byte* begin{};
    std::byte* end{};
    size_t pageIndex{};
};

class Heap
{
#if FRAZE_HEAP_DEBUG
    SourceLocation* pLocation{};
    std::ofstream logFile;
#endif

    std::mutex mut;
    std::vector<Page> pages;
    std::vector<PageInfo> sortedPageInfo;
    std::byte* minAddress = nullptr;
    std::byte* maxAddress = nullptr;

    std::vector<std::span<std::byte>> ranges;
    std::unordered_set<const std::byte*> pinned;
    uint8_t currentColor{};
    Program* pProgram{};
public:
    static constexpr size_t BlockSize = 16U;
    static constexpr size_t MinPageSize = 8 * 1024;
    static constexpr size_t MinGrownPageSize = 256 * 1024;
    static constexpr uint8_t InitialBlockColor = 0;

    static_assert(MinPageSize >= BlockSize);
    static_assert(MinGrownPageSize >= MinPageSize);

    Heap(Program* pProgram);

    std::byte* Allocate(size_t size);
    void Collect();
    size_t TotalUsed() const;
    void Report();
    void AddRange(std::span<std::byte> range);
    void RemoveRange(std::byte* rangeStart);
    void PinMemory(const std::byte* p);
    void UnpinMemory(const std::byte* p);

#if FRAZE_HEAP_DEBUG
    void SetLocation(SourceLocation* pLocation);
#endif

private:
    std::byte* AllocateRaw(size_t size);
    std::byte* AllocateFromExistingPages(size_t size);
    Page* AddPage(size_t requestedSize);
    void RemovePage(size_t pageIndex);
    Page* FindPage(const std::byte* p);
    void CollectInternal();
    void ScanRange(std::span<std::byte> range);
    void Mark(const std::byte* p);

#if FRAZE_HEAP_DEBUG
    void LogLocation(const char* tag, const SourceLocation* pLoc);
#endif
};

struct Page
{
    struct AlignedDeleter {
        void operator()(std::byte* p) const noexcept {
            ::operator delete[](p, std::align_val_t(Heap::BlockSize));
        }
    };

    size_t totalSize{};
    size_t blockCount{};
    size_t totalUsed{};
    size_t nextFree{};
    std::unique_ptr<std::byte[], AlignedDeleter> storage;
    std::vector<uint8_t> starts;
    std::vector<uint8_t> inuse;
    std::vector<uint8_t> color;
    static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

    Page(size_t requestedSize = Heap::MinPageSize);

    std::byte* Allocate(size_t size);
    void Deallocate(const std::byte* p);
    bool Contains(const std::byte* p) const;
    size_t GetHeapObjectStart(const std::byte* p) const;
    HeapObject* GetHeapObject(size_t blockIndex);
};

} // fraze

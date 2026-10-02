/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <fraze/common/DynamicArray.h>
#include <fraze/common/Exception.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <fraze/common/Pointers.h>
#include <fraze/memory/Heap.h>
#include <fraze/memory/DefaultAllocator.h>
#include <fraze/program/CheckSite.h>
#include <fraze/program/TypeInfo.h>

namespace fraze {

class ScopedAllocator;

// A compiled program: the heap, type info, globals and other run-time state that every backend shares. A backend
// derives from it and implements InvokeImpl and GetUsedStackRange.
class Program
{
    Heap heap;

    friend DefaultAllocator;
    friend ScopedAllocator;
    friend Heap;
public:
    std::vector<std::unique_ptr<Object, Object::Deleter>> staticObjects;
    std::vector<CheckSite> checkSites;
    std::vector<sptr<TypeInfo>> typeInfo;
    dynamic_array<Word> globals;

    Program();
    virtual ~Program() = default;

    Word Invoke(const std::string& qualifiedFuncName, WordValue auto&&... args)
    {
        std::array<Word, sizeof...(args)> words {
            Word(std::forward<decltype(args)>(args))...
        };
        return InvokeImpl(qualifiedFuncName, words);
    }

    TypeInfo* GetTypeInfo(std::string_view qualifiedName);
    void PinMemory(const void* p);
    void UnpinMemory(const void* p);
    void UnpinMemory(const std::span<std::byte*> ps);
    void Collect();
    void Report();

    template<ObjectSubclass T, typename... Args>
        requires std::is_base_of_v<Object, T>
    Object* NewExternClass(const std::string& qualifiedName, Args&&... args)
    {
        T* obj = reinterpret_cast<T*>(heap.Allocate(sizeof(T), true));
        std::construct_at(obj, std::forward<Args>(args)...);
        obj->info = GetTypeInfo(qualifiedName);
        assert(obj->info);
        return obj;
    }

protected:
    virtual Word InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args) = 0;

    // Returns the words of the backend's stack that are in use, for the GC to scan as roots.
    virtual std::span<Word> GetUsedStackRange() = 0;
};

} // fraze

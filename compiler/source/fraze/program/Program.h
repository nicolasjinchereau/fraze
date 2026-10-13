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
#include <fraze/memory/Allocator.h>
#include <fraze/program/CheckSite.h>
#include <fraze/program/TypeInfo.h>

namespace fraze {

class PersistentAllocator;
class ScopedAllocator;

using CompiledFunctionCall = Word(void* functionAddress, CompiledResultKind resultKind, Word* args);

class Program
{
protected:

    friend Allocator;
    friend PersistentAllocator;
    friend ScopedAllocator;
    friend Heap;
public:
    Heap heap;
    std::vector<CheckSite> checkSites;
    std::vector<sptr<TypeInfo>> typeInfo;
    dynamic_array<Word> globals;

    Program();
    virtual ~Program() = default;

    Word Invoke(const std::string& qualifiedFuncName, WordValue auto&&... args);

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
    // 'callCompiledFunction' is for a program whose functions are machine code. The VM runs 'args' as they are.
    virtual Word InvokeImpl(
        const std::string& qualifiedFuncName,
        const std::span<Word>& args,
        CompiledFunctionCall* callCompiledFunction) = 0;

    // Heap::CollectInternal scans this range for references.
    virtual std::span<Word> GetUsedStackRange() = 0;
};


// A num is passed to compiled code in a floating-point register, and everything else in an integer register.
template<class Arg>
using CompiledArg = std::conditional_t<std::is_floating_point_v<Arg>, Number, Integer>;

template<class Arg>
inline CompiledArg<Arg> GetCompiledArg(const Word& word)
{
    if constexpr(std::is_floating_point_v<Arg>)
        return word.GetNumber();
    else
        return word.GetInteger();
}

template<class Result, class... Args, size_t... ArgIndices>
inline Result CallCompiledFunctionAs(void* functionAddress, Word* args, std::index_sequence<ArgIndices...>)
{
    auto function = reinterpret_cast<Result(*)(CompiledArg<Args>...)>(functionAddress);
    return function(GetCompiledArg<Args>(args[ArgIndices])...);
}

// Calls a compiled function with the words in 'args', each passed as the type the host gave for it in 'Args'.
// Program::Invoke hands a JITProgram the instance of this for its own argument types.
template<class... Args>
inline Word CallCompiledFunction(void* functionAddress, CompiledResultKind resultKind, Word* args)
{
    constexpr auto argIndices = std::index_sequence_for<Args...>();

    switch(resultKind)
    {
    case CompiledResultKind::None:
        CallCompiledFunctionAs<void, Args...>(functionAddress, args, argIndices);
        return Word(nullptr);

    case CompiledResultKind::Number:
        return Word(CallCompiledFunctionAs<Number, Args...>(functionAddress, args, argIndices));

    case CompiledResultKind::Integer:
        return Word(CallCompiledFunctionAs<Integer, Args...>(functionAddress, args, argIndices));
    }

    std::unreachable();
}

inline Word Program::Invoke(const std::string& qualifiedFuncName, WordValue auto&&... args)
{
    std::array<Word, sizeof...(args)> words{
        Word(std::forward<decltype(args)>(args))...
    };
    return InvokeImpl(qualifiedFuncName, words, &CallCompiledFunction<std::remove_cvref_t<decltype(args)>...>);
}

} // fraze

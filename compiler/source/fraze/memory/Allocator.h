#/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <fraze/common/Platform.h>

namespace fraze {

class Program;
class Object;
class Class;
class String;
struct TypeInfo;

template<class T>
class Array;

#if FRAZE_HEAP_DEBUG
#  define FRAZE_HEAP_DEBUG_LOC , std::source_location loc = std::source_location::current()
#  define FRAZE_HEAP_DEBUG_LOC_PARAM , std::source_location loc
#  define FRAZE_HEAP_DEBUG_LOC_ARG , loc
#else
#  define FRAZE_HEAP_DEBUG_LOC
#  define FRAZE_HEAP_DEBUG_LOC_PARAM
#  define FRAZE_HEAP_DEBUG_LOC_ARG
#endif

class Allocator
{
protected:
    Program* _program{};
public:
    Allocator(Program* program) : _program(program) {}
    ~Allocator() {}

    virtual std::byte* Allocate(size_t size);

    Program* program() {
        return _program;
    }

    String* NewString(std::string_view value FRAZE_HEAP_DEBUG_LOC);
    String* NewString(size_t length FRAZE_HEAP_DEBUG_LOC);
    String* NewString(std::string_view left, std::string_view right FRAZE_HEAP_DEBUG_LOC);
    Array<void>* NewArray(const std::string& qualifiedTypeName, size_t count FRAZE_HEAP_DEBUG_LOC);
    Class* NewClass(const std::string& qualifiedTypeName FRAZE_HEAP_DEBUG_LOC);

    template<class T>
    Array<T>* NewArray(const std::string& qualifiedTypeName, size_t count FRAZE_HEAP_DEBUG_LOC) {
        return static_cast<Array<T>*>(NewArray(qualifiedTypeName, count FRAZE_HEAP_DEBUG_LOC_ARG));
    }

    template<class T>
    T* NewExternClass(const std::string& qualifiedName FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG);
    }

    template<class T, class A1>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1));
    }

    template<class T, class A1, class A2>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1, A2&& a2 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1), std::forward<A2>(a2));
    }

    template<class T, class A1, class A2, class A3>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1, A2&& a2, A3&& a3 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3));
    }

    template<class T, class A1, class A2, class A3, class A4>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1, A2&& a2, A3&& a3, A4&& a4 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3), std::forward<A4>(a4));
    }

    template<class T, class A1, class A2, class A3, class A4, class A5>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3), std::forward<A4>(a4), std::forward<A5>(a5));
    }

    template<class T, class A1, class A2, class A3, class A4, class A5, class A6>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3), std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6));
    }

    template<class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    T* NewExternClass(const std::string& qualifiedName, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7 FRAZE_HEAP_DEBUG_LOC) {
        return NewExternClassImpl<T>(qualifiedName FRAZE_HEAP_DEBUG_LOC_ARG,
            std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3), std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6), std::forward<A7>(a7));
    }

private:
    TypeInfo* GetTypeInfo(const std::string& qualifiedName);
    std::byte* AllocateExternClass(size_t size FRAZE_HEAP_DEBUG_LOC_PARAM);

    template<class T, typename... Args> requires std::is_base_of_v<Object, T>
    T* NewExternClassImpl(const std::string& qualifiedName FRAZE_HEAP_DEBUG_LOC_PARAM, Args&&... args)
    {
        T* obj = reinterpret_cast<T*>(AllocateExternClass(sizeof(T) FRAZE_HEAP_DEBUG_LOC_ARG));
        TypeInfo* typeInfo = GetTypeInfo(qualifiedName);
        std::construct_at(obj, typeInfo, std::forward<Args>(args)...);
        assert(typeInfo);
        return obj;
    }
};

} // fraze

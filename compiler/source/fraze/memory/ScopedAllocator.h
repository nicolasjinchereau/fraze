#/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <span>
#include <string>
#include <vector>
#include <fraze/common/Object.h>
#include <fraze/common/SourceLocation.h>
#include <fraze/memory/IAllocator.h>

namespace fraze {

class Program;
class Object;
struct TypeInfo;

class ScopedAllocator : public IAllocator
{
    std::vector<std::byte*> allocated;
public:
    ScopedAllocator(Program* program);
    ~ScopedAllocator();

    ScopedAllocator(const ScopedAllocator&) = delete;
    ScopedAllocator& operator=(const ScopedAllocator&) = delete;

    ScopedAllocator(ScopedAllocator&& other) noexcept
        : IAllocator(other._program)
        , allocated(std::move(other.allocated))
    {
        other._program = nullptr;
    }

    ScopedAllocator& operator=(ScopedAllocator&& other) noexcept
    {
        if (this != &other)
        {
            allocated = std::move(other.allocated);
            _program = other._program;
            other._program = nullptr;
        }
        return *this;
    }

    virtual std::byte* Allocate(size_t size) override;
    static TypeInfo* GetTypeInfo(Program* program, const std::string& qualifiedName);

    static String* NewString(IAllocator& allocator, std::string_view value, SourceLocation* pLoc = nullptr);
    static String* NewString(IAllocator& allocator, size_t length, SourceLocation* pLoc = nullptr);
    static Array<>* NewArray(IAllocator& allocator, const std::string& qualifiedTypeName, size_t count, SourceLocation* pLoc = nullptr);
    static Class* NewClass(IAllocator& allocator, const std::string& qualifiedTypeName, SourceLocation* pLoc = nullptr);

    template<class T>
    static Array<T>* NewArray(IAllocator& allocator, const std::string& qualifiedTypeName, size_t count, SourceLocation* pLoc = nullptr) {
        return static_cast<Array<T>*>(NewArray(allocator, qualifiedTypeName, count, pLoc));
    }

    template<ObjectSubclass T, typename... Args> requires std::is_base_of_v<Object, T>
    static T* NewExternClass(IAllocator& allocator, const std::string& qualifiedName, SourceLocation* pLoc, Args&&... args)
    {
        T* obj = reinterpret_cast<T*>(allocator.Allocate(sizeof(T)));
        TypeInfo* typeInfo = GetTypeInfo(allocator.program(), qualifiedName);
        std::construct_at(obj, typeInfo, std::forward<Args>(args)...);
        assert(typeInfo);
        return obj;
    }
};

#if FRAZE_HEAP_DEBUG
#define NEW_FRAZE_STRING(allocator, value) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return fraze::ScopedAllocator::NewString((allocator), (value), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_STRING_N(allocator, length) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return fraze::ScopedAllocator::NewString((allocator), (length), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_ARRAY(allocator, qualifiedTypeName, count) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return fraze::ScopedAllocator::NewArray((allocator), (qualifiedTypeName), (count), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_ARRAY_T(allocator, type, qualifiedTypeName, count) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return fraze::ScopedAllocator::NewArray<type>((allocator), (qualifiedTypeName), (count), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_CLASS(allocator, qualifiedTypeName) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return fraze::ScopedAllocator::NewClass((allocator), (qualifiedTypeName), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_EXTERN_CLASS(allocator, type, qualifiedTypeName, ...) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return fraze::ScopedAllocator::NewExternClass<type>((allocator), (qualifiedTypeName), &CONCAT(loc_, __LINE__) __VA_OPT__(,) __VA_ARGS__); \
}()
#else
#define NEW_FRAZE_STRING(allocator, value) \
(fraze::ScopedAllocator::NewString((allocator), (value)))

#define NEW_FRAZE_STRING_N(allocator, length) \
(fraze::ScopedAllocator::NewString((allocator), (length)))

#define NEW_FRAZE_ARRAY(allocator, qualifiedTypeName, count) \
(fraze::ScopedAllocator::NewArray((allocator), (qualifiedTypeName), (count)))

#define NEW_FRAZE_ARRAY_T(allocator, type, qualifiedTypeName, count) \
(fraze::ScopedAllocator::NewArray<type>((allocator), (qualifiedTypeName), (count)))

#define NEW_FRAZE_CLASS(allocator, qualifiedTypeName) \
(fraze::ScopedAllocator::NewClass((allocator), (qualifiedTypeName)))

#define NEW_FRAZE_EXTERN_CLASS(allocator, type, qualifiedTypeName, ...) \
(fraze::ScopedAllocator::NewExternClass<type>((allocator), (qualifiedTypeName), nullptr __VA_OPT__(,) __VA_ARGS__))
#endif

} // fraze

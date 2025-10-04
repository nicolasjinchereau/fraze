#/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <span>
#include <string>
#include <vector>
#include <fraze/common/Object.h>
#include <fraze/common/SourceLocation.h>

namespace fraze {

class Program;
class Object;

class ScopedAllocator
{
    Program* program{};
    std::vector<Object*> allocated;
public:
    ScopedAllocator(Program* program);
    ~ScopedAllocator();

    String* NewString(std::string_view value, SourceLocation* pLoc = nullptr);
    String* NewString(size_t length, SourceLocation* pLoc = nullptr);
    Array<>* NewArray(const std::string& qualifiedTypeName, size_t count, SourceLocation* pLoc = nullptr);
    Class* NewClass(const std::string& qualifiedTypeName, SourceLocation* pLoc = nullptr); //, std::span<Word> args, std::span<int> offsets);

    template<class T>
    Array<T>* NewArray(const std::string& qualifiedTypeName, size_t count, SourceLocation* pLoc = nullptr) {
        return static_cast<Array<T>*>( NewArray(qualifiedTypeName, count, pLoc) );
    }
};

#if FRAZE_HEAP_DEBUG
#define NEW_FRAZE_STRING(allocator, value) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return (allocator).NewString((value), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_STRING_N(allocator, length) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return (allocator).NewString((length), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_ARRAY(allocator, type, qualifiedTypeName, count) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return (allocator).NewArray<type>((qualifiedTypeName), (count), &CONCAT(loc_, __LINE__)); \
}()

#define NEW_FRAZE_CLASS(allocator, qualifiedTypeName) \
[&]{ static fraze::SourceLocation CONCAT(loc_, __LINE__) = fraze::SourceLocation(std::source_location::current()); \
    return (allocator).NewClass((qualifiedTypeName), &CONCAT(loc_, __LINE__)); \
}()
#else
#define NEW_FRAZE_STRING(allocator, value) \
((allocator).NewString((value)))

#define NEW_FRAZE_STRING_N(allocator, length) \
((allocator).NewString((length)))

#define NEW_FRAZE_ARRAY(allocator, type, qualifiedTypeName, count) \
((allocator).NewArray<type>((qualifiedTypeName), (count)))

#define NEW_FRAZE_CLASS(allocator, qualifiedTypeName) \
((allocator).NewClass((qualifiedTypeName)))
#endif

} // fraze

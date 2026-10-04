/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/Allocator.h>
#include <fraze/common/Object.h>
#include <fraze/program/Program.h>

namespace fraze {

std::byte* Allocator::Allocate(size_t size)
{
    return program()->heap.Allocate(size, false);
}

String* Allocator::NewString(std::string_view value FRAZE_HEAP_DEBUG_LOC_PARAM)
{
#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(loc);
#endif

    String* ret = String::New(*this, value);

#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(nullptr);
#endif

    return ret;
}

String* Allocator::NewString(size_t length FRAZE_HEAP_DEBUG_LOC_PARAM)
{
#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(loc);
#endif

    String* ret = String::New(*this, length);

#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(nullptr);
#endif

    return ret;
}

String* Allocator::NewString(std::string_view left, std::string_view right FRAZE_HEAP_DEBUG_LOC_PARAM)
{
#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(loc);
#endif

    String* ret = String::New(*this, left, right);

#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(nullptr);
#endif

    return ret;
}

Array<>* Allocator::NewArray(const std::string& qualifiedTypeName, size_t count FRAZE_HEAP_DEBUG_LOC_PARAM)
{
#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(loc);
#endif

    TypeInfo* typeInfo = program()->GetTypeInfo(qualifiedTypeName);
    ArrayInfo* arrayType = typeInfo ? typeInfo->ToArrayInfo() : nullptr;
    ENFORCE(!!arrayType, SourceLocation(), "type not found: {}", qualifiedTypeName);

    size_t wordCount = count * arrayType->GetElementSize();
    Array<>* ret =  Array<>::New(*this, arrayType, wordCount);

#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(nullptr);
#endif

    return ret;
}

Class* Allocator::NewClass(const std::string& qualifiedTypeName FRAZE_HEAP_DEBUG_LOC_PARAM)
{
#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(loc);
#endif

    TypeInfo* typeInfo = program()->GetTypeInfo(qualifiedTypeName);
    ClassInfo* classType = typeInfo ? typeInfo->ToClassInfo() : nullptr;
    ENFORCE(!!classType, SourceLocation(), "type not found: {}", qualifiedTypeName);

    Class* ret = Class::New(*this, classType);

    // call init function...

#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(nullptr);
#endif

    return ret;
}

TypeInfo* Allocator::GetTypeInfo(const std::string& qualifiedName)
{
    return program()->GetTypeInfo(qualifiedName);
}

std::byte* Allocator::AllocateExternClass(size_t size FRAZE_HEAP_DEBUG_LOC_PARAM)
{
#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(loc);
#endif

    std::byte* ret = Allocate(size);

#if FRAZE_HEAP_DEBUG
    program()->heap.SetLocation(nullptr);
#endif

    return ret;
}

} // fraze

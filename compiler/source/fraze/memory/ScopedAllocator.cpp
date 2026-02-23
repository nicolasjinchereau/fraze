#/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/ScopedAllocator.h>
#include <fraze/program/Program.h>
#include <fraze/common/Platform.h>

namespace fraze {


ScopedAllocator::ScopedAllocator(Program* program)
    : IAllocator(program)
{
}

ScopedAllocator::~ScopedAllocator()
{
    program()->UnpinMemory(allocated);
}

std::byte* ScopedAllocator::Allocate(size_t size)
{
    std::byte* p = program()->heap.Allocate(size, true);
    allocated.push_back(p);
    return p;
}

TypeInfo* ScopedAllocator::GetTypeInfo(Program* program, const std::string& qualifiedName)
{
    return program->GetTypeInfo(qualifiedName);
}

String* ScopedAllocator::NewString(IAllocator& allocator, std::string_view value, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    String* ret = String::New(allocator, value);

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

String* ScopedAllocator::NewString(IAllocator& allocator, size_t length, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    String* ret = String::New(allocator, length);

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

Array<>* ScopedAllocator::NewArray(IAllocator& allocator, const std::string& qualifiedTypeName, size_t count, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif
    
    TypeInfo* typeInfo = allocator.program()->GetTypeInfo(qualifiedTypeName);
    ArrayInfo* arrayType = typeInfo ? typeInfo->ToArrayInfo() : nullptr;
    ENFORCE(!!arrayType, SourceLocation(), "type not found: {}", qualifiedTypeName);
    
    size_t wordCount = count * arrayType->GetElementSize();
    Array<>* ret =  Array<>::New(allocator, arrayType, wordCount);

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

Class* ScopedAllocator::NewClass(IAllocator& allocator, const std::string& qualifiedTypeName, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    TypeInfo* typeInfo = allocator.program()->GetTypeInfo(qualifiedTypeName);
    ClassInfo* classType = typeInfo ? typeInfo->ToClassInfo() : nullptr;
    ENFORCE(!!classType, SourceLocation(), "type not found: {}", qualifiedTypeName);

    Class* ret = Class::New(allocator, classType);

    // call init function...

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

} // fraze

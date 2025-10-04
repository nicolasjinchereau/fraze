#/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/memory/ScopedAllocator.h>
#include <fraze/program/Program.h>
#include <fraze/common/Platform.h>

namespace fraze {


ScopedAllocator::ScopedAllocator(Program* program)
    : program(program)
{
}

ScopedAllocator::~ScopedAllocator()
{
    for(auto& obj : allocated)
    {
        program->UnpinMemory(obj);
    }
}

String* ScopedAllocator::NewString(std::string_view value, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    String* ret = String::New(program->heap, value);
    program->PinMemory(ret);
    allocated.push_back(ret);

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

String* ScopedAllocator::NewString(size_t length, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    String* ret = String::New(program->heap, length);
    program->PinMemory(ret);
    allocated.push_back(ret);

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

Array<>* ScopedAllocator::NewArray(const std::string& qualifiedTypeName, size_t count, SourceLocation* pLoc)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    TypeInfo* typeInfo = program->GetTypeInfo(qualifiedTypeName);
    ArrayInfo* arrayType = typeInfo ? typeInfo->ToArrayInfo() : nullptr;
    ENFORCE(!!arrayType, SourceLocation(), "type not found: {}", qualifiedTypeName);
    
    size_t wordCount = count * arrayType->GetElementSize();
    Array<>* ret =  static_cast<Array<>*>( fraze::Array<>::New(program->heap, arrayType, wordCount) );
    program->PinMemory(ret);
    allocated.push_back(ret);

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

Class* ScopedAllocator::NewClass(const std::string& qualifiedTypeName, SourceLocation* pLoc)//, std::span<Word> args, std::span<int> offsets)
{
#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(pLoc);
#endif

    TypeInfo* typeInfo = program->GetTypeInfo(qualifiedTypeName);
    ClassInfo* classType = typeInfo ? typeInfo->ToClassInfo() : nullptr;
    ENFORCE(!!classType, SourceLocation(), "type not found: {}", qualifiedTypeName);

    Class* ret = Class::New(program->heap, classType);
    program->PinMemory(ret);
    allocated.push_back(ret);

    // call init function...

#if FRAZE_HEAP_DEBUG
    program->heap.SetLocation(nullptr);
#endif

    return ret;
}

} // fraze

/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/Program.h>

namespace fraze {

Program::Program()
    : heap(this)
{
}

TypeInfo* Program::GetTypeInfo(std::string_view qualifiedName)
{
    for(auto& ti : typeInfo)
    {
        if(ti->qualifiedName == qualifiedName)
            return ti.get();
    }

    return nullptr;
}

void Program::Report()
{
    heap.Report();
}

void Program::PinMemory(const void* p)
{
    heap.PinMemory(static_cast<const std::byte*>(p));
}

void Program::UnpinMemory(const void* p)
{
    heap.UnpinMemory(static_cast<const std::byte*>(p));
}

void Program::UnpinMemory(const std::span<std::byte*> ps)
{
    heap.UnpinMemory(ps);
}

void Program::Collect()
{
    heap.Collect();
}

} // fraze

#/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstddef>

namespace fraze {

class Program;

class IAllocator
{
protected:
    Program* _program{};
public:
    IAllocator(Program* program) : _program(program) {}
    ~IAllocator() {}

    virtual std::byte* Allocate(size_t size) = 0;

    Program* program() {
        return _program;
    }
};

} // fraze

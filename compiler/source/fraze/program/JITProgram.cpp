/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/JITProgram.h>
#include <fraze/common/Exception.h>
#include <fraze/common/SourceLocation.h>
#include <mir.h>

namespace fraze {

JITProgram::JITProgram()
{
    context = MIR_init();
}

JITProgram::~JITProgram()
{
    MIR_finish(context);
}

void* JITProgram::GetFunctionAddress(const std::string& qualifiedFuncName)
{
    auto it = functionAddresses.find(qualifiedFuncName);
    ENFORCE(it != functionAddresses.end(), SourceLocation(), "function not found: {}", qualifiedFuncName);
    return it->second;
}

Word JITProgram::InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args)
{
    if(shouldInitialize)
    {
        shouldInitialize = false;
        GetFunction<void()>("$staticConstructor")();
    }

    ENFORCE(args.empty(), SourceLocation(), "the JIT can't pass arguments yet: {}", qualifiedFuncName);

    return Word(GetFunction<Integer()>(qualifiedFuncName)());
}

// Compiled code can't allocate yet, so there are no references on the native stack.
std::span<Word> JITProgram::GetUsedStackRange()
{
    return {};
}

} // fraze

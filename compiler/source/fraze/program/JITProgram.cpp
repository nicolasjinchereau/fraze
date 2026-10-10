/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/JITProgram.h>
#include <fraze/common/Exception.h>
#include <fraze/common/SourceLocation.h>
#include <fraze/common/jump/LandingPad.h>
#include <mir.h>

namespace fraze {

void* JITProgram::GetFunctionAddress(const std::string& qualifiedFuncName)
{
    auto it = functionAddresses.find(qualifiedFuncName);
    ENFORCE(it != functionAddresses.end(), SourceLocation(), "function not found: {}", qualifiedFuncName);
    return it->second;
}

// Everything that can throw on its own is done before the pad is saved, so the pad covers the calls into
// compiled code and nothing else.
Word JITProgram::InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args)
{
    ENFORCE(args.empty(), SourceLocation(), "the JIT can't pass arguments yet: {}", qualifiedFuncName);

    auto staticConstructor = shouldInitialize ? GetFunction<void()>("$staticConstructor") : nullptr;
    auto function = GetFunction<Integer()>(qualifiedFuncName);

    LandingPad pad;

    if(SaveJumpPoint(pad.jumpPoint) != 0)
        pad.RethrowFailure();

    if(staticConstructor)
    {
        shouldInitialize = false;
        staticConstructor();
    }

    return Word(function());
}

// Compiled code can't allocate yet, so there are no references on the native stack.
std::span<Word> JITProgram::GetUsedStackRange()
{
    return {};
}

} // fraze

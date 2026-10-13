/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/JITProgram.h>
#include <fraze/common/Exception.h>
#include <fraze/common/SourceLocation.h>
#include <fraze/common/jump/LandingPad.h>
#include <mir.h>

namespace fraze {

// Returns the function a host names, which is the first one with that name, as in VMProgram. Throws when the
// host can't invoke it.
FunctionInfo* JITProgram::GetCompiledFunctionInfo(const std::string& qualifiedFuncName)
{
    TypeInfo* typeInfo = GetTypeInfo(qualifiedFuncName);
    FunctionInfo* functionInfo = typeInfo ? typeInfo->ToFunctionInfo() : nullptr;

    ENFORCE(functionInfo && functionInfo->functionAddress, SourceLocation(),
        "function not found: {}", qualifiedFuncName);

    ENFORCE(functionInfo->compiledResultKind != CompiledResultKind::Unsupported, SourceLocation(),
        "a host can't invoke a function that takes or returns a struct: {}", qualifiedFuncName);

    return functionInfo;
}

// Everything that can throw on its own is done before the pad is saved, so the pad covers the calls into
// compiled code and nothing else.
Word JITProgram::InvokeImpl(
    const std::string& qualifiedFuncName,
    const std::span<Word>& args,
    CompiledFunctionCall* callCompiledFunction)
{
    FunctionInfo* staticConstructorInfo = shouldInitialize ? GetCompiledFunctionInfo("$staticConstructor") : nullptr;
    FunctionInfo* functionInfo = GetCompiledFunctionInfo(qualifiedFuncName);

    ENFORCE(functionInfo->paramSize == args.size(), SourceLocation(), "wrong number of args: {}", args.size());

    LandingPad pad;

    if(SaveJumpPoint(pad.jumpPoint) != 0)
        pad.RethrowFailure();

    if(staticConstructorInfo)
    {
        shouldInitialize = false;
        heap.RegisterExecutionThread();
        reinterpret_cast<void(*)()>(staticConstructorInfo->functionAddress)();
    }

    return callCompiledFunction(functionInfo->functionAddress, functionInfo->compiledResultKind, args.data());
}

// Returns the part of the program's stack that is in use. A JIT program has no stack of its own, so this is
// always empty.
std::span<Word> JITProgram::GetUsedStackRange()
{
    return {};
}

} // fraze

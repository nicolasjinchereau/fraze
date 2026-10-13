/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <span>
#include <string>
#include <fraze/common/Object.h>
#include <fraze/common/Pointers.h>
#include <fraze/program/MIRContext.h>
#include <fraze/program/Program.h>

namespace fraze {

class JITProgram : public Program
{
    bool shouldInitialize = true;

    FunctionInfo* GetCompiledFunctionInfo(const std::string& qualifiedFuncName);

public:
    sptr<MIRContext> context = spnew<MIRContext>();

protected:
    Word InvokeImpl(
        const std::string& qualifiedFuncName,
        const std::span<Word>& args,
        CompiledFunctionCall* callCompiledFunction) override;
    std::span<Word> GetUsedStackRange() override;
};

} // fraze

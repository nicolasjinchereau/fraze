/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <span>
#include <string>
#include <unordered_map>
#include <fraze/common/Object.h>
#include <fraze/common/Pointers.h>
#include <fraze/program/MIRContext.h>
#include <fraze/program/Program.h>

namespace fraze {

class JITProgram : public Program
{
    bool shouldInitialize = true;

    void* GetFunctionAddress(const std::string& qualifiedFuncName);

    template<class Signature>
    Signature* GetFunction(const std::string& qualifiedFuncName) {
        return reinterpret_cast<Signature*>(GetFunctionAddress(qualifiedFuncName));
    }

public:
    sptr<MIRContext> context = spnew<MIRContext>();
    std::unordered_map<std::string, void*> functionAddresses;

protected:
    Word InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args) override;
    std::span<Word> GetUsedStackRange() override;
};

} // fraze

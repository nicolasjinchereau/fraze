/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <span>
#include <string>
#include <unordered_map>
#include <fraze/common/Object.h>
#include <fraze/program/Program.h>

struct MIR_context;

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
    MIR_context* context{};
    std::unordered_map<std::string, void*> functionAddresses;

    JITProgram();
    ~JITProgram() override;

protected:
    Word InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args) override;
    std::span<Word> GetUsedStackRange() override;
};

} // fraze

/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <fraze/common/DynamicArray.h>
#include <fraze/common/Exception.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Stack.h>
#include <fraze/common/Utility.h>
#include <fraze/memory/Heap.h>
#include <fraze/memory/DefaultAllocator.h>
#include <fraze/program/CheckSite.h>
#include <fraze/program/OpCode.h>
#include <fraze/program/Operation.h>
#include <fraze/program/Program.h>
#include <fraze/program/TypeInfo.h>
#include <fraze/common/ExternalFunction.h>

namespace fraze {

class VMProgram : public Program
{
    constexpr static std::size_t StackSize = 1024 * 1024 / sizeof(Word);

    dynamic_array<Word, Heap::BlockSize> stack;
    // The base pointer, stack pointer and instruction pointer as of the last time Run stored them. Run works on local
    // copies and stores all three before an external call and when it finishes, for callbacks to read, and rsp after
    // every operation, for the GC.
    Word* rbp = nullptr;
    Word* rsp = nullptr;
    size_t rip = 0;
    bool shouldInitialize = true;

    std::size_t GetStackSize() const;

public:
    std::vector<Word> data;
    std::vector<WordType> dataTypes;
    std::vector<Operation> code;
    std::vector<SourceLocation> locations; // of operations in 'code'

    VMProgram();

protected:
    Word InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args) override;
    std::span<Word> GetUsedStackRange() override;

private:
    void Run();

    FRAZE_INLINE void Execute_NoOp(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushLiteral(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushLocal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushLocalN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushLocalAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopLocal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopLocalN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushGlobal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushGlobalAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopGlobal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushArgument(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushArgumentN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushArgumentAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopArgument(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushWord(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushWordN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushWordAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopWord(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopWordN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushIndexAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushOffset(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PopOffset(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushBoolean(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushInteger(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushNumber(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_PushNull(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Pop(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Reserve(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LogicalOr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LogicalAnd(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_BitOr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_BitXor(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_BitAnd(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_BitNot(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LeftShift(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_RightShift(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Equal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_EqualN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_NotEqual(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_NotEqualN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LessInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LessNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LessEqualInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_LessEqualNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_GreaterInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_GreaterNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_GreaterEqualInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_GreaterEqualNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_AddInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_AddNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_SubInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_SubNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_MulInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_MulNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_DivInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_DivNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_ModInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_ModNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_ConvIntToNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_ConvNumToInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Dup(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_DupN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Call(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_CallVirtual(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Return(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_CallExternal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Jump(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_JumpIf(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_JumpIfNot(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
    FRAZE_INLINE void Execute_Switch(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip);
};

} // fraze

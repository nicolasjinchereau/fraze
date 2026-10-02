/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/program/VMProgram.h>
#include <fraze/program/ProgramDiagnostics.h>
#include <fraze/common/ExternalFunction.h>
#include <fraze/common/Platform.h>
#include <ranges>
#include <tuple>
#include <print>

namespace fraze {

constexpr size_t DONE_INSTR = static_cast<size_t>(-1);

VMProgram::VMProgram()
{
    stack = dynamic_array<Word, Heap::BlockSize>(StackSize);
    rsp = stack.data() - 1;
    rbp = stack.data();
    rip = 0;
}

std::size_t VMProgram::GetStackSize() const {
    return std::size_t(rsp + 1 - stack.data());
}

// Returns the VM stack from its bottom up to rsp, which Run stores after every operation.
std::span<Word> VMProgram::GetUsedStackRange()
{
    return std::span(stack.data(), rsp + 1);
}

Word VMProgram::InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args)
{
    if(shouldInitialize)
    {
        shouldInitialize = false;
        InvokeImpl("$staticConstructor", {});
    }

    auto typeInfo = GetTypeInfo(qualifiedFuncName);
    ENFORCE(typeInfo != nullptr, SourceLocation(), "function not found: {}", qualifiedFuncName);

    auto funcInfo = typeInfo->ToFunctionInfo();
    ENFORCE(funcInfo->paramSize == args.size(), SourceLocation(), "wrong number of args: {}", args.size());

#ifndef NDEBUG
    size_t previousStackSize = GetStackSize();
    Word* previousBasePointer = rbp;
#endif

    // return storage
    rsp += funcInfo->returnSize;

    // args
    for(auto& arg : std::views::reverse(args))
        *(++rsp) = arg;

// Call + Prologue
    // Push a sentinel value instead of the instruction pointer
    // since we have no real return address here in C++.
    std::size_t previousInstructionPointer = rip;
    *(++rsp) = Word::Raw(DONE_INSTR);
    rip = funcInfo->codeStart;

    // save/bump base pointer and allocate locals
    *(++rsp) = Word(rbp);
    rbp = rsp + 1;

    // allocate locals
    rsp += funcInfo->localSize;

    Run();

    // OpCode::Return restored the sentinel from the stack,
    // so restore the real instruction pointer here.
    rip = previousInstructionPointer;

    assert(rbp == previousBasePointer);

    // OpCode::Return will pop rbp and args
    
    // void functions have no return storage. Assume at most a 1-word return value for now.
    Word result = funcInfo->returnSize != 0 ? *(rsp--) : Word(nullptr);

    assert(GetStackSize() == previousStackSize);

    return result;
}

#define FRAZE_EXECUTE_CASE(name) case OpCode::name: Execute_##name(op, rsp, rbp, rip); break;

// Executes operations from the instruction pointer, rip, until a Return jumps to the DONE_INSTR sentinel. It copies
// the stack pointer, base pointer and instruction pointer into locals of the same names and passes them by reference
// to the handlers, which are force-inlined, so the C++ compiler can keep all three in machine registers. It stores rsp
// back after every operation, for the GC, and all three when it finishes; Execute_CallExternal stores all three before
// calling native code.
void VMProgram::Run()
{
    Word* rsp = this->rsp;
    Word* rbp = this->rbp;
    size_t rip = this->rip;

    const Operation* ops = code.data();

    while(rip != DONE_INSTR)
    {
        const Operation& op = ops[rip];

#if FRAZE_PRINT_EXECUTED_CODE
        ProgramDiagnostics::PrintExecutedOperation(*this, rip);
#endif

#if FRAZE_HEAP_DEBUG
        heap.SetLocation(&locations[rip]);
#endif

#if FRAZE_CODE_PROFILING
        ProgramDiagnostics::BeginOperationMeasurement();
#endif // FRAZE_CODE_PROFILING

        switch(op.code)
        {
        FRAZE_EXECUTE_CASE(NoOp)
        FRAZE_EXECUTE_CASE(PushLiteral)
        FRAZE_EXECUTE_CASE(PushLocal)
        FRAZE_EXECUTE_CASE(PushLocalN)
        FRAZE_EXECUTE_CASE(PushLocalAddr)
        FRAZE_EXECUTE_CASE(PopLocal)
        FRAZE_EXECUTE_CASE(PopLocalN)
        FRAZE_EXECUTE_CASE(PushGlobal)
        FRAZE_EXECUTE_CASE(PushGlobalAddr)
        FRAZE_EXECUTE_CASE(PopGlobal)
        FRAZE_EXECUTE_CASE(PushArgument)
        FRAZE_EXECUTE_CASE(PushArgumentN)
        FRAZE_EXECUTE_CASE(PushArgumentAddr)
        FRAZE_EXECUTE_CASE(PopArgument)
        FRAZE_EXECUTE_CASE(PushWord)
        FRAZE_EXECUTE_CASE(PushWordN)
        FRAZE_EXECUTE_CASE(PushWordAddr)
        FRAZE_EXECUTE_CASE(PopWord)
        FRAZE_EXECUTE_CASE(PopWordN)
        FRAZE_EXECUTE_CASE(PushIndexAddr)
        FRAZE_EXECUTE_CASE(PushOffset)
        FRAZE_EXECUTE_CASE(PopOffset)
        FRAZE_EXECUTE_CASE(PushBoolean)
        FRAZE_EXECUTE_CASE(PushInteger)
        FRAZE_EXECUTE_CASE(PushNumber)
        FRAZE_EXECUTE_CASE(PushNull)
        FRAZE_EXECUTE_CASE(Pop)
        FRAZE_EXECUTE_CASE(Reserve)
        FRAZE_EXECUTE_CASE(LogicalOr)
        FRAZE_EXECUTE_CASE(LogicalAnd)
        FRAZE_EXECUTE_CASE(BitOr)
        FRAZE_EXECUTE_CASE(BitXor)
        FRAZE_EXECUTE_CASE(BitAnd)
        FRAZE_EXECUTE_CASE(BitNot)
        FRAZE_EXECUTE_CASE(LeftShift)
        FRAZE_EXECUTE_CASE(RightShift)
        FRAZE_EXECUTE_CASE(Equal)
        FRAZE_EXECUTE_CASE(EqualN)
        FRAZE_EXECUTE_CASE(NotEqual)
        FRAZE_EXECUTE_CASE(NotEqualN)
        FRAZE_EXECUTE_CASE(LessInt)
        FRAZE_EXECUTE_CASE(LessNum)
        FRAZE_EXECUTE_CASE(LessEqualInt)
        FRAZE_EXECUTE_CASE(LessEqualNum)
        FRAZE_EXECUTE_CASE(GreaterInt)
        FRAZE_EXECUTE_CASE(GreaterNum)
        FRAZE_EXECUTE_CASE(GreaterEqualInt)
        FRAZE_EXECUTE_CASE(GreaterEqualNum)
        FRAZE_EXECUTE_CASE(AddInt)
        FRAZE_EXECUTE_CASE(AddNum)
        FRAZE_EXECUTE_CASE(SubInt)
        FRAZE_EXECUTE_CASE(SubNum)
        FRAZE_EXECUTE_CASE(MulInt)
        FRAZE_EXECUTE_CASE(MulNum)
        FRAZE_EXECUTE_CASE(DivInt)
        FRAZE_EXECUTE_CASE(DivNum)
        FRAZE_EXECUTE_CASE(ModInt)
        FRAZE_EXECUTE_CASE(ModNum)
        FRAZE_EXECUTE_CASE(ConvIntToNum)
        FRAZE_EXECUTE_CASE(ConvNumToInt)
        FRAZE_EXECUTE_CASE(Dup)
        FRAZE_EXECUTE_CASE(DupN)
        FRAZE_EXECUTE_CASE(Call)
        FRAZE_EXECUTE_CASE(CallVirtual)
        FRAZE_EXECUTE_CASE(Return)
        FRAZE_EXECUTE_CASE(CallExternal)
        FRAZE_EXECUTE_CASE(Jump)
        FRAZE_EXECUTE_CASE(JumpIf)
        FRAZE_EXECUTE_CASE(JumpIfNot)
        FRAZE_EXECUTE_CASE(Switch)
        default:
            assert(!"every OpCode needs a case");
            __assume(0);
        }

#if FRAZE_CODE_PROFILING
        ProgramDiagnostics::EndOperationMeasurement(op.code);
#endif // FRAZE_CODE_PROFILING

#if FRAZE_HEAP_DEBUG
        heap.SetLocation(nullptr);
#endif
        // Ensure the GC can see a mostly up-to-date stack pointer to
        // mitigate the risk of miscollection until we can stop the world.
        this->rsp = rsp;
    }

    this->rsp = rsp;
    this->rbp = rbp;
    this->rip = rip;
}

#undef FRAZE_EXECUTE_CASE

void VMProgram::Execute_NoOp(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    ++rip;
}

void VMProgram::Execute_PushLiteral(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    *(++rsp) = *(data.data() + op.arg1_u64);
    ++rip;
}

void VMProgram::Execute_PushLocal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 == 0);
    *(++rsp) = *(rbp + op.arg1_u64);
    ++rip;
}

void VMProgram::Execute_PushLocalN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* src = rbp + op.arg1_u64;
    Word* dest = rsp + 1;

    for(std::size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp = dest + (op.arg2_u64 - 1);
    ++rip;
}

void VMProgram::Execute_PushLocalAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    *(++rsp) = Word(rbp + op.arg1_u64);
    ++rip;
}

void VMProgram::Execute_PopLocal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 == 0);
    *(rbp + op.arg1_u64) = *(rsp--);
    ++rip;
}

void VMProgram::Execute_PopLocalN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 != 0);
    
    auto dest = rbp + op.arg1_u64;
    auto src = rsp + 1 - op.arg2_u64;
    
    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp -= op.arg2_u64;
    ++rip;
}

void VMProgram::Execute_PushGlobal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 != 0);

    Word* dest = rsp + 1;
    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = this->globals[ op.arg1_u64 + i ];

    rsp = dest + (op.arg2_u64 - 1);
    ++rip;
}

void VMProgram::Execute_PushGlobalAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    *(++rsp) = &this->globals[ op.arg1_u64 ];
    ++rip;
}

void VMProgram::Execute_PopGlobal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Word* global = &this->globals[ op.arg1_u64 ];
    Word* value = top + 1 - op.arg2_u64;

    for(size_t i = 0; i != op.arg2_u64; ++i)
        global[i] = value[i];

    rsp = top - op.arg2_u64;
    ++rip;
}

void VMProgram::Execute_PushArgument(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;
    
    assert(argSize == 0);

    *(++rsp) = *(rbp - 3 - argIndex);
    ++rip;
}

void VMProgram::Execute_PushArgumentN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;

    assert(argSize != 0);

    Word* src = rbp - 3 - argIndex - (argSize - 1);
    Word* dest = rsp + 1;

    for(std::size_t i = 0; i != argSize; ++i)
        dest[i] = src[i];

    rsp = dest + (argSize - 1);
    ++rip;
}

void VMProgram::Execute_PushArgumentAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;

    *(++rsp) = { rbp - 3 - argIndex - (argSize - 1) };
    ++rip;
}

void VMProgram::Execute_PopArgument(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;

    assert(argSize != 0);

    Word* top = rsp;
    auto arg = rbp - 3 - argIndex - (argSize - 1);
    auto value = top + 1 - argSize;

    for(size_t i = 0; i != argSize; ++i)
        arg[i] = value[i];

    rsp = top - argSize;
    ++rip;
}

void VMProgram::Execute_PushWord(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Reference address = rsp->reference;
    *rsp = address[op.arg1_u64];
    ++rip;
}

void VMProgram::Execute_PushWordN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Reference address = (top--)->reference;

    Word* src = address + op.arg1_u64;
    Word* dest = top + 1;

    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp = dest + (op.arg2_u64 - 1);
    ++rip;
}

void VMProgram::Execute_PushWordAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Reference address = rsp->reference;
    rsp->reference = address + op.arg1_u64;
    ++rip;
}

void VMProgram::Execute_PopWord(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Reference address = (top--)->reference;
    address[op.arg1_u64] = *top;
    rsp = top - 1;
    ++rip;
}

void VMProgram::Execute_PopWordN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Reference address = (top--)->reference;

    Word* src = top + 1 - op.arg2_u64;
    Word* dest = address + op.arg1_u64;

    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp = top - op.arg2_u64;
    ++rip;
}

void VMProgram::Execute_PushIndexAddr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer index = (top--)->integer;
    Reference address = top->reference;
    top->reference = address + op.arg1_u64 + index * static_cast<Integer>(op.arg2_u64);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_PushOffset(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    auto valueRef = top - op.arg1_u64;

    if(op.arg2_u64 != 0)
    {
        for(size_t i = 0; i != op.arg2_u64; ++i)
            *(++top) = valueRef[i];
    }
    else // push a reference to the var at offset
    {
        *(++top) = valueRef;
    }

    rsp = top;
    ++rip;
}

void VMProgram::Execute_PopOffset(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    *(rsp - op.arg1_u64) = *(rsp--);
    ++rip;
}

void VMProgram::Execute_PushBoolean(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    (++rsp)->storage = static_cast<uint64_t>(op.arg1_i64 != 0);
    ++rip;
}

void VMProgram::Execute_PushInteger(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    (++rsp)->integer = op.arg1_i64;
    ++rip;
}

void VMProgram::Execute_PushNumber(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    (++rsp)->number = op.arg1_f64;
    ++rip;
}

void VMProgram::Execute_PushNull(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    (++rsp)->object = nullptr;
    ++rip;
}

void VMProgram::Execute_Pop(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    rsp -= op.arg1_u64;
    ++rip;
}

void VMProgram::Execute_Reserve(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    rsp += op.arg1_u64;
    ++rip;
}

void VMProgram::Execute_LogicalOr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Boolean rhs = static_cast<Boolean>((top--)->storage);
    Boolean lhs = static_cast<Boolean>((top--)->storage);
    (++top)->storage = static_cast<uint64_t>(lhs || rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_LogicalAnd(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Boolean rhs = static_cast<Boolean>((top--)->storage);
    Boolean lhs = static_cast<Boolean>((top--)->storage);
    (++top)->storage = static_cast<uint64_t>(lhs && rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_BitOr(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs | rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_BitXor(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs ^ rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_BitAnd(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs & rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_BitNot(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    top->integer = ~top->integer;
    ++rip;
}

void VMProgram::Execute_LeftShift(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs << rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_RightShift(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs >> rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_Equal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg1_u64 == 0);
    Word* top = rsp;
    uint64_t rhs = (top--)->storage;
    uint64_t lhs = top->storage;
    top->storage = (lhs == rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_EqualN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg1_u64 > 1);

    Word* rhs = rsp + 1 - op.arg1_u64;
    Word* lhs = rhs - op.arg1_u64;

    uint64_t i = 0;

    for( ; i != op.arg1_u64; ++i)
    {
        if(rhs[i] != lhs[i])
            break;
    }

    lhs->storage = static_cast<uint64_t>(i == op.arg1_u64);
    rsp = lhs;
    ++rip;
}

void VMProgram::Execute_NotEqual(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg1_u64 == 0);
    Word* top = rsp;
    uint64_t rhs = (top--)->storage;
    uint64_t lhs = top->storage;
    top->storage = (lhs != rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_NotEqualN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(op.arg1_u64 > 1);

    Word* rhs = rsp + 1 - op.arg1_u64;
    Word* lhs = rhs - op.arg1_u64;

    uint64_t i = 0;

    for( ; i != op.arg1_u64; ++i)
    {
        if(rhs[i] != lhs[i])
            break;
    }

    lhs->storage = static_cast<uint64_t>(i != op.arg1_u64);
    rsp = lhs;
    ++rip;
}

void VMProgram::Execute_LessInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs < rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_LessNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs < rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_LessEqualInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs <= rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_LessEqualNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs <= rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_GreaterInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs > rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_GreaterNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs > rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_GreaterEqualInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs >= rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_GreaterEqualNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs >= rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_AddInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs + rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_AddNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs + rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_SubInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs - rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_SubNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs - rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_MulInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs * rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_MulNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs * rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_DivInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs / rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_DivNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs / rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_ModInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs % rhs;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_ModNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = fmod(lhs, rhs);
    rsp = top;
    ++rip;
}

void VMProgram::Execute_ConvIntToNum(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    rsp->number = static_cast<Number>(rsp->integer);
    ++rip;
}

void VMProgram::Execute_ConvNumToInt(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    rsp->integer = static_cast<Integer>(rsp->number);
    ++rip;
}

void VMProgram::Execute_Dup(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    *(++top) = *top;
    rsp = top;
    ++rip;
}

void VMProgram::Execute_DupN(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp;
    Word* src = top + 1 - op.arg1_u64;
    Word* dest = top + 1;

    for (std::size_t i = 0; i < op.arg1_u64; ++i)
        dest[i] = src[i];

    rsp = dest + (op.arg1_u64 - 1);
    ++rip;
}

void VMProgram::Execute_Call(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    auto info = typeInfo[op.arg1_u64]->ToFunctionInfo();
    assert(info);

    Word* top = rsp;

    *(++top) = Word::Raw(rip);
    rip = info->codeStart;

    *(++top) = { rbp };
    rbp = top + 1;
    rsp = top + info->localSize;
}

void VMProgram::Execute_CallVirtual(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Word* top = rsp; // starts at context pointer

    const uint64_t interfaceFuncID = op.arg1_u64;
    const uint64_t interfaceType = op.arg2_u64;

    auto interfaceFuncInfo = typeInfo[interfaceFuncID]->ToFunctionInfo();
    assert(interfaceFuncInfo);
    
    Class* obj = top->GetClass();
    size_t actualFuncID = obj->GetFunctionID(interfaceType, interfaceFuncInfo->offset);

    auto info = typeInfo[actualFuncID]->ToFunctionInfo();
    assert(info);

    *(++top) = Word::Raw(rip);
    rip = info->codeStart;

    *(++top) = { rbp };
    rbp = top + 1;
    rsp = top + info->localSize;
}

void VMProgram::Execute_Return(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    const uint64_t argsSize = op.arg1_u64;
    const uint64_t returnSize = op.arg2_u64;

    Word* top = rsp;

    Word* returnStorageStart = rbp - 2 - argsSize - returnSize;
    Word* returnValueEnd = top + 1;
    Word* returnValueStart = returnValueEnd - returnSize;

    while(returnValueStart != returnValueEnd)
    {
        *(returnStorageStart++) = *(returnValueStart++);
    }

    top = rbp - 1;
    rbp = (top--)->reference;

    rip = (top--)->storage;

    rsp = top - argsSize;

    if(rip != DONE_INSTR)
        ++rip;
}

// Calls the native function on the arguments at the top of the stack, then pops them. It first stores the stack
// pointer, base pointer and instruction pointer into the rsp, rbp and rip members, where a callback into the VM and the
// GC read them. It pushes no frame, so a callback builds its frame above the arguments, and InvokeImpl saves rip.
void VMProgram::Execute_CallExternal(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    assert(typeInfo[op.arg1_u64]->ToFunctionInfo());
    auto info = static_cast<const FunctionInfo*>(typeInfo[op.arg1_u64].get());

    this->rsp = rsp;
    this->rbp = rbp;
    this->rip = rip;

    info->externalFunction->Invoke(this, rsp + 1);

    rsp -= info->paramSize;
    ++rip;
}

void VMProgram::Execute_Jump(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    rip = op.arg1_u64;
}

void VMProgram::Execute_JumpIf(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Boolean value = static_cast<Boolean>((rsp--)->storage);
    if(value)
        rip = op.arg1_u64;
    else
        ++rip;
}

void VMProgram::Execute_JumpIfNot(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    Boolean value = static_cast<Boolean>((rsp--)->storage);
    if(!value)
        rip = op.arg1_u64;
    else
        ++rip;
}

void VMProgram::Execute_Switch(const Operation& op, Word*& rsp, Word*& rbp, size_t& rip)
{
    // unsigned, so a value below the table wraps around to past its end
    uint64_t entry = static_cast<uint64_t>((rsp--)->integer) - op.arg1_u64;
    rip += 1 + std::min(entry, op.arg2_u64);
}

} // fraze
 

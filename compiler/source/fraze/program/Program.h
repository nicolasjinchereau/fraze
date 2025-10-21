/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <fraze/common/Exception.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Stack.h>
#include <fraze/common/Utility.h>
#include <fraze/memory/Heap.h>
#include <fraze/program/OpCode.h>
#include <fraze/program/TypeInfo.h>
#include <fraze/common/ExternalFunction.h>

namespace fraze {

struct Operation
{
    OpCode code;

    union {
        uint64_t arg1_u64;
        struct {
            uint32_t arg1_u32a;
            uint32_t arg1_u32b;
        };
        Integer arg1_i64;
        Number arg1_f64;
        const char* arg1_cstr;
    };

    union {
        uint64_t arg2_u64;
        struct {
            uint32_t arg2_u32a;
            uint32_t arg2_u32b;
        };
        Integer arg2_i64;
        Number arg2_f64;
        const char* arg2_cstr;
    };

    SourceLocation loc;

    Operation(const SourceLocation& loc, OpCode op, auto arg1, auto arg2)
        : code(op), loc(loc)
    {
        if constexpr(std::is_floating_point_v<decltype(arg1)>)
            arg1_f64 = arg1;
        else if constexpr(std::is_unsigned_v<decltype(arg1)>)
            arg1_u64 = arg1;
        else if constexpr(std::is_signed_v<decltype(arg1)>)
            arg1_i64 = arg1;
        else if constexpr(is_c_string_v<decltype(arg1)>)
            arg1_cstr = arg1;
        else
            static_assert(0);

        if constexpr(std::is_floating_point_v<decltype(arg2)>)
            arg2_f64 = arg2;
        else if constexpr(std::is_unsigned_v<decltype(arg2)>)
            arg2_u64 = arg2;
        else if constexpr(std::is_signed_v<decltype(arg2)>)
            arg2_i64 = arg2;
        else if constexpr(is_c_string_v<decltype(arg2)>)
            arg2_cstr = arg2;
        else
            static_assert(0);
    }

    Operation(const SourceLocation& loc, OpCode op, auto arg1)
        : Operation(loc, op, arg1, 0ull)
    {
    }

    Operation(const SourceLocation& loc, OpCode op)
        : Operation(loc, op, 0ull, 0ull)
    {
    }
};


struct StackFrame
{
    Word* start{};
    uint32_t returnSize = 1;
    uint32_t paramCount = 0;
    bool hasContext = false;

    bool operator==(const StackFrame&) const = default;
    bool operator!=(const StackFrame&) const = default;
};

class ScopedAllocator;

class Program
{
    Heap heap;
    fraze::stack<Word> stack;
    fraze::stack<StackFrame> stackFrames;
    fraze::stack<size_t> codePointers;

    bool initialized = false;
    void Initialize();

    friend ScopedAllocator;
public:
    std::vector<std::unique_ptr<Object, Object::Deleter>> staticObjects;
    std::vector<Word> data;
    std::vector<WordType> dataTypes;
    std::vector<Operation> code;
    std::vector<sptr<TypeInfo>> typeInfo;
    std::vector<IntrinsicFunction> intrinsics;
    size_t globalCount{};
    
    Program();

    Word Invoke(const std::string& qualifiedFuncName, WordValue auto&&... args)
    {
        std::array<Word, sizeof...(args)> words {
            Word(std::forward<decltype(args)>(args))...
        };
        return InvokeImpl(qualifiedFuncName, words);
    }

    TypeInfo* GetTypeInfo(const std::string& qualifiedName);
    void Execute(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void PinMemory(const void* p);
    void UnpinMemory(const void* p);
    void Collect();
    void Report();
    void Print(bool printData, bool printCode);
    void PrintOperation(size_t index, std::ostream& stream);
    std::string GetLiteralValue(uint64_t index);

private:
    Word InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args);

    using Handler = void (Program::*)(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_NoOp(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushLiteral(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushContext(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushLocal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushLocalN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushLocalAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopLocal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopLocalN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushGlobal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushGlobalAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopGlobal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushArgument(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushArgumentN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushArgumentAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopArgument(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushFieldAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushRefField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushRefFieldN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushRefFieldAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopRefField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushElement(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushElementAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopElement(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushOffset(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PopOffset(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushBoolean(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushInteger(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushNumber(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushNull(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushCount(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_PushSize(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Pop(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Reserve(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_NewArray(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_NewClass(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LogicalOr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LogicalAnd(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_BitOr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_BitXor(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_BitAnd(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_BitNot(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LeftShift(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_RightShift(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Equal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_EqualN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_StringEqual(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_IsInstance(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LessInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LessNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LessEqualInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_LessEqualNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_GreaterInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_GreaterNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_GreaterEqualInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_GreaterEqualNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_AddInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_AddNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_SubInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_SubNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_MulInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_MulNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_DivInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_DivNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ModInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ModNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvIntToNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvNumToInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvBoolToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvIntToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvNumToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvEnumToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvObjToType(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ConvRefToStruct(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Box(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Unbox(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_StringConcat(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Dup(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_DupN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Call(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_CallExternal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_CallIntrinsic(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_CallVirtual(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Jump(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_JumpIf(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_JumpIfNot(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Goto(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Return(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_Assert(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_NullCheck(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_BoundsCheck(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    void Execute_ObjectTypeCheck(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip);
    
    // static asserts to check handler index against OpCode values
    void VerifyHandlers();

    static constexpr Handler handlers[static_cast<size_t>(OpCode::COUNT)] = {
        &Program::Execute_NoOp,
        &Program::Execute_PushLiteral,
        &Program::Execute_PushContext,
        &Program::Execute_PushLocal,
        &Program::Execute_PushLocalN,
        &Program::Execute_PushLocalAddr,
        &Program::Execute_PopLocal,
        &Program::Execute_PopLocalN,
        &Program::Execute_PushGlobal,
        &Program::Execute_PushGlobalAddr,
        &Program::Execute_PopGlobal,
        &Program::Execute_PushArgument,
        &Program::Execute_PushArgumentN,
        &Program::Execute_PushArgumentAddr,
        &Program::Execute_PopArgument,
        &Program::Execute_PushField,
        &Program::Execute_PushFieldAddr,
        &Program::Execute_PopField,
        &Program::Execute_PushRefField,
        &Program::Execute_PushRefFieldN,
        &Program::Execute_PushRefFieldAddr,
        &Program::Execute_PopRefField,
        &Program::Execute_PushElement,
        &Program::Execute_PushElementAddr,
        &Program::Execute_PopElement,
        &Program::Execute_PushOffset,
        &Program::Execute_PopOffset,
        &Program::Execute_PushBoolean,
        &Program::Execute_PushInteger,
        &Program::Execute_PushNumber,
        &Program::Execute_PushNull,
        &Program::Execute_PushCount,
        &Program::Execute_PushSize,
        &Program::Execute_Pop,
        &Program::Execute_Reserve,
        &Program::Execute_NewArray,
        &Program::Execute_NewClass,
        &Program::Execute_LogicalOr,
        &Program::Execute_LogicalAnd,
        &Program::Execute_BitOr,
        &Program::Execute_BitXor,
        &Program::Execute_BitAnd,
        &Program::Execute_BitNot,
        &Program::Execute_LeftShift,
        &Program::Execute_RightShift,
        &Program::Execute_Equal,
        &Program::Execute_EqualN,
        &Program::Execute_StringEqual,
        &Program::Execute_IsInstance,
        &Program::Execute_LessInt,
        &Program::Execute_LessNum,
        &Program::Execute_LessEqualInt,
        &Program::Execute_LessEqualNum,
        &Program::Execute_GreaterInt,
        &Program::Execute_GreaterNum,
        &Program::Execute_GreaterEqualInt,
        &Program::Execute_GreaterEqualNum,
        &Program::Execute_AddInt,
        &Program::Execute_AddNum,
        &Program::Execute_SubInt,
        &Program::Execute_SubNum,
        &Program::Execute_MulInt,
        &Program::Execute_MulNum,
        &Program::Execute_DivInt,
        &Program::Execute_DivNum,
        &Program::Execute_ModInt,
        &Program::Execute_ModNum,
        &Program::Execute_ConvIntToNum,
        &Program::Execute_ConvNumToInt,
        &Program::Execute_ConvBoolToStr,
        &Program::Execute_ConvIntToStr,
        &Program::Execute_ConvNumToStr,
        &Program::Execute_ConvEnumToStr,
        &Program::Execute_ConvObjToType,
        &Program::Execute_ConvRefToStruct,
        &Program::Execute_Box,
        &Program::Execute_Unbox,
        &Program::Execute_StringConcat,
        &Program::Execute_Dup,
        &Program::Execute_DupN,
        &Program::Execute_Call,
        &Program::Execute_CallExternal,
        &Program::Execute_CallIntrinsic,
        &Program::Execute_CallVirtual,
        &Program::Execute_Jump,
        &Program::Execute_JumpIf,
        &Program::Execute_JumpIfNot,
        &Program::Execute_Goto,
        &Program::Execute_Return,
        &Program::Execute_Assert,
        &Program::Execute_NullCheck,
        &Program::Execute_BoundsCheck,
        &Program::Execute_ObjectTypeCheck,
    };

};

} // fraze

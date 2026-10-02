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
#include <fraze/program/TypeInfo.h>
#include <fraze/common/ExternalFunction.h>

namespace fraze {

class ScopedAllocator;

class Program
{
    constexpr static std::size_t StackSize = 1024 * 1024 / sizeof(Word);

    Heap heap;
    dynamic_array<Word, Heap::BlockSize> stack;
    Word* rbp = nullptr; // base pointer
    Word* rsp = nullptr;
    size_t rip = 0;
    bool shouldInitialize = true;

    std::size_t GetStackSize() const;

    friend DefaultAllocator;
    friend ScopedAllocator;
    friend Heap;
public:
    std::vector<std::unique_ptr<Object, Object::Deleter>> staticObjects;
    std::vector<Word> data;
    std::vector<WordType> dataTypes;
    std::vector<Operation> code;
    std::vector<SourceLocation> locations; // of operations in 'code'
    std::vector<CheckSite> checkSites;
    std::vector<sptr<TypeInfo>> typeInfo;
    dynamic_array<Word> globals;

    Program();

    Word Invoke(const std::string& qualifiedFuncName, WordValue auto&&... args)
    {
        std::array<Word, sizeof...(args)> words {
            Word(std::forward<decltype(args)>(args))...
        };
        return InvokeImpl(qualifiedFuncName, words);
    }

    TypeInfo* GetTypeInfo(std::string_view qualifiedName);
    void Execute(const Operation& op);
    void PinMemory(const void* p);
    void UnpinMemory(const void* p);
    void UnpinMemory(const std::span<std::byte*> ps);
    void Collect();
    void Report();

    template<ObjectSubclass T, typename... Args>
        requires std::is_base_of_v<Object, T>
    Object* NewExternClass(const std::string& qualifiedName, Args&&... args)
    {
        T* obj = reinterpret_cast<T*>(heap.Allocate(sizeof(T), true));
        std::construct_at(obj, std::forward<Args>(args)...);
        obj->info = GetTypeInfo(qualifiedName);
        assert(obj->info);
        return obj;
    }

private:
    Word InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args);

    using Handler = void (Program::*)(const Operation& op);
    void Execute_NoOp(const Operation& op);
    void Execute_PushLiteral(const Operation& op);
    void Execute_PushLocal(const Operation& op);
    void Execute_PushLocalN(const Operation& op);
    void Execute_PushLocalAddr(const Operation& op);
    void Execute_PopLocal(const Operation& op);
    void Execute_PopLocalN(const Operation& op);
    void Execute_PushGlobal(const Operation& op);
    void Execute_PushGlobalAddr(const Operation& op);
    void Execute_PopGlobal(const Operation& op);
    void Execute_PushArgument(const Operation& op);
    void Execute_PushArgumentN(const Operation& op);
    void Execute_PushArgumentAddr(const Operation& op);
    void Execute_PopArgument(const Operation& op);
    void Execute_PushWord(const Operation& op);
    void Execute_PushWordN(const Operation& op);
    void Execute_PushWordAddr(const Operation& op);
    void Execute_PopWord(const Operation& op);
    void Execute_PopWordN(const Operation& op);
    void Execute_PushIndexAddr(const Operation& op);
    void Execute_PushOffset(const Operation& op);
    void Execute_PopOffset(const Operation& op);
    void Execute_PushBoolean(const Operation& op);
    void Execute_PushInteger(const Operation& op);
    void Execute_PushNumber(const Operation& op);
    void Execute_PushNull(const Operation& op);
    void Execute_Pop(const Operation& op);
    void Execute_Reserve(const Operation& op);
    void Execute_LogicalOr(const Operation& op);
    void Execute_LogicalAnd(const Operation& op);
    void Execute_BitOr(const Operation& op);
    void Execute_BitXor(const Operation& op);
    void Execute_BitAnd(const Operation& op);
    void Execute_BitNot(const Operation& op);
    void Execute_LeftShift(const Operation& op);
    void Execute_RightShift(const Operation& op);
    void Execute_Equal(const Operation& op);
    void Execute_EqualN(const Operation& op);
    void Execute_NotEqual(const Operation& op);
    void Execute_NotEqualN(const Operation& op);
    void Execute_LessInt(const Operation& op);
    void Execute_LessNum(const Operation& op);
    void Execute_LessEqualInt(const Operation& op);
    void Execute_LessEqualNum(const Operation& op);
    void Execute_GreaterInt(const Operation& op);
    void Execute_GreaterNum(const Operation& op);
    void Execute_GreaterEqualInt(const Operation& op);
    void Execute_GreaterEqualNum(const Operation& op);
    void Execute_AddInt(const Operation& op);
    void Execute_AddNum(const Operation& op);
    void Execute_SubInt(const Operation& op);
    void Execute_SubNum(const Operation& op);
    void Execute_MulInt(const Operation& op);
    void Execute_MulNum(const Operation& op);
    void Execute_DivInt(const Operation& op);
    void Execute_DivNum(const Operation& op);
    void Execute_ModInt(const Operation& op);
    void Execute_ModNum(const Operation& op);
    void Execute_ConvIntToNum(const Operation& op);
    void Execute_ConvNumToInt(const Operation& op);
    void Execute_Dup(const Operation& op);
    void Execute_DupN(const Operation& op);
    void Execute_Call(const Operation& op);
    void Execute_CallVirtual(const Operation& op);
    void Execute_Return(const Operation& op);
    void Execute_CallExternal(const Operation& op);
    void Execute_Jump(const Operation& op);
    void Execute_JumpIf(const Operation& op);
    void Execute_JumpIfNot(const Operation& op);
    void Execute_Switch(const Operation& op);

    static constexpr Handler handlers[static_cast<size_t>(OpCode::COUNT)] = {
        &Program::Execute_NoOp,
        &Program::Execute_PushLiteral,
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
        &Program::Execute_PushWord,
        &Program::Execute_PushWordN,
        &Program::Execute_PushWordAddr,
        &Program::Execute_PopWord,
        &Program::Execute_PopWordN,
        &Program::Execute_PushIndexAddr,
        &Program::Execute_PushOffset,
        &Program::Execute_PopOffset,
        &Program::Execute_PushBoolean,
        &Program::Execute_PushInteger,
        &Program::Execute_PushNumber,
        &Program::Execute_PushNull,
        &Program::Execute_Pop,
        &Program::Execute_Reserve,
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
        &Program::Execute_NotEqual,
        &Program::Execute_NotEqualN,
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
        &Program::Execute_Dup,
        &Program::Execute_DupN,
        &Program::Execute_Call,
        &Program::Execute_CallVirtual,
        &Program::Execute_Return,
        &Program::Execute_CallExternal,
        &Program::Execute_Jump,
        &Program::Execute_JumpIf,
        &Program::Execute_JumpIfNot,
        &Program::Execute_Switch,
    };

    static_assert(handlers[std::size(handlers) - 1] != nullptr, "every OpCode needs a handler");
};

} // fraze

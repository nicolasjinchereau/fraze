/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>

namespace fraze
{

enum class OpCode : uint8_t
{
    // do nothing. Args will be printed to output for debugging
    NoOp,

    // Push a literal at DATA[arg] onto stack
    PushLiteral,

    // Push STACK[rbp + arg] onto stack
    PushLocal,
    PushLocalN,
    PushLocalAddr,

    // Pop stack into STACK[rbp + arg]
    PopLocal,
    PopLocalN,

    // Push GLOBAL[arg] onto stack
    PushGlobal,
    PushGlobalAddr,

    // Pop stack into GLOBAL[arg]
    PopGlobal,

    // Push STACK[frameStart - paramCount + arg] onto stack
    PushArgument,
    PushArgumentN,
    PushArgumentAddr,

    // Pop stack into STACK[frameStart - paramCount + arg]
    PopArgument,

    // Direct memory access. These all treat the word on top of the stack as the address
    // of a block of words, and access that block at a constant word offset that is baked
    // in at compile time. A struct value on the stack is already the address of its
    // fields, while a class or array object needs Class/ArrayDataOffset added to reach
    // past its header.

    // pop an address, then push the word at [address + arg1]
    PushWord,

    // pop an address, then push the arg2 words starting at [address + arg1]
    PushWordN,

    // pop an address, then push [address + arg1]
    PushWordAddr,

    // pop an address and the value below it, then store the value at [address + arg1]
    PopWord,

    // pop an address and the arg2 values below it, then store them at [address + arg1]
    PopWordN,

    // Indexed direct memory access. Computes an address as [address + disp + index * scale],
    // which is the same shape as an x86 [base + index * scale + disp] addressing mode and a
    // MIR memory operand. It is kept separate from the load/store operations above so that
    // every operation stays within two constant operands; a JIT tracking the operand stack
    // symbolically can fold this into the displacement of the following load or store for
    // free, so the split costs an interpreter dispatch but no machine instructions.

    // pop an index and the address below it, then push [address + arg1 + index * arg2]
    PushIndexAddr,

    // Push STACK[STACK.size - 1 - arg] onto stack
    PushOffset,

    // Pop stack into STACK[STACK.size - 1 - arg]
    PopOffset,

    // Push a boolean onto the stack, in-line
    PushBoolean,

    // Push an integer number onto the stack, in-line
    PushInteger,

    // Push a number onto the stack, in-line
    PushNumber,

    // Push a null reference to the stack
    PushNull,

    // Pop stack and discard
    Pop,

    // Reserve space for return value, arg is word count
    Reserve,

    // replaces stack top with result of binary operation on top two (integers)
    LogicalOr,
    LogicalAnd,
    BitOr,
    BitXor,
    BitAnd,
    BitNot,
    LeftShift,
    RightShift,

    // replaces stack top with result of equality/inequality operation on top two (any Word, arg=size)
    Equal,
    EqualN,
    NotEqual,
    NotEqualN,

    // replaces stack top with result of equality operation on top two
    LessInt,
    LessNum,
    LessEqualInt,
    LessEqualNum,
    GreaterInt,
    GreaterNum,
    GreaterEqualInt,
    GreaterEqualNum,

    // replaces stack top with result of binary operation on top two
    AddInt,
    AddNum,
    SubInt,
    SubNum,
    MulInt,
    MulNum,
    DivInt,
    DivNum,
    ModInt,
    ModNum,

    // converts stack top from one type to another
    ConvIntToNum,
    ConvNumToInt,

    // pushes another copy of current stack top
    Dup,
    DupN,

    Call, // calls function id on top of stack
    CallVirtual, // stack top is interface function id and arg is interface id
    Return, // Pop rip

    // calls external function id on top of stack
    CallExternal,
    CallIntrinsic,
    
    Jump, // jump to code[arg]
    JumpIf, // jump to code[arg] if stack[top] is true and pop stack
    JumpIfNot, // jump to code[arg] if stack[top] is false and pop stack

    // jump to stack[top]
    Goto,

    // number of enum members
    COUNT,
};

inline std::unordered_map<OpCode, std::string> OpCodeNames {
    { OpCode::NoOp,            "NoOp" },
    { OpCode::PushLiteral,     "PushLiteral" },
    { OpCode::PushArgument,    "PushArgument" },
    { OpCode::PushArgumentN,   "PushArgumentN" },
    { OpCode::PushArgumentAddr,"PushArgumentAddr" },
    { OpCode::PopArgument,     "PopArgument" },
    { OpCode::PushLocal,       "PushLocal" },
    { OpCode::PushLocalN,      "PushLocalN" },
    { OpCode::PushLocalAddr,   "PushLocalAddr" },
    { OpCode::PopLocal,        "PopLocal" },
    { OpCode::PopLocalN,       "PopLocalN" },
    { OpCode::PushOffset,      "PushOffset" },
    { OpCode::PopOffset,       "PopOffset" },
    { OpCode::PushGlobal,      "PushGlobal" },
    { OpCode::PushGlobalAddr,  "PushGlobalAddr" },
    { OpCode::PopGlobal,       "PopGlobal" },
    { OpCode::PushWord,        "PushWord" },
    { OpCode::PushWordN,       "PushWordN" },
    { OpCode::PushWordAddr,    "PushWordAddr" },
    { OpCode::PopWord,         "PopWord" },
    { OpCode::PopWordN,        "PopWordN" },
    { OpCode::PushIndexAddr,   "PushIndexAddr" },
    { OpCode::PushBoolean,     "PushBoolean" },
    { OpCode::PushInteger,     "PushInteger" },
    { OpCode::PushNumber,      "PushNumber" },
    { OpCode::PushNull,        "PushNull" },
    { OpCode::Pop,             "Pop" },
    { OpCode::Reserve,         "Reserve" },

    { OpCode::LogicalOr,       "LogicalOr" },
    { OpCode::LogicalAnd,      "LogicalAnd" },
    { OpCode::BitOr,           "BitOr" },
    { OpCode::BitXor,          "BitXor" },
    { OpCode::BitAnd,          "BitAnd" },
    { OpCode::BitNot,          "BitNot" },
    { OpCode::LeftShift,       "LeftShift" },
    { OpCode::RightShift,      "RightShift" },

    { OpCode::Equal,           "Equal" },
    { OpCode::EqualN,          "EqualN" },
    { OpCode::NotEqual,        "NotEqual" },
    { OpCode::NotEqualN,       "NotEqualN" },

    { OpCode::LessInt,         "LessInt" },
    { OpCode::LessNum,         "LessNum" },
    { OpCode::LessEqualInt,    "LessEqualInt" },
    { OpCode::LessEqualNum,    "LessEqualNum" },
    { OpCode::GreaterInt,      "GreaterInt" },
    { OpCode::GreaterNum,      "GreaterNum" },
    { OpCode::GreaterEqualInt, "GreaterEqualInt" },
    { OpCode::GreaterEqualNum, "GreaterEqualNum" },

    { OpCode::AddInt,          "AddInt" },
    { OpCode::AddNum,          "AddNum" },
    { OpCode::SubInt,          "SubInt" },
    { OpCode::SubNum,          "SubNum" },
    { OpCode::MulInt,          "MulInt" },
    { OpCode::MulNum,          "MulNum" },
    { OpCode::DivInt,          "DivInt" },
    { OpCode::DivNum,          "DivNum" },
    { OpCode::ModInt,          "ModInt" },
    { OpCode::ModNum,          "ModNum" },

    { OpCode::ConvIntToNum,    "ConvIntToNum" },
    { OpCode::ConvNumToInt,    "ConvNumToInt" },

    { OpCode::Dup,             "Dup" },
    { OpCode::DupN,            "DupN" },
    { OpCode::Call,            "Call" },
    { OpCode::CallExternal,    "CallExternal" },
    { OpCode::CallIntrinsic,   "CallIntrinsic" },
    { OpCode::CallVirtual,     "CallVirtual" },
    { OpCode::Jump,            "Jump" },
    { OpCode::JumpIf,          "JumpIf" },
    { OpCode::JumpIfNot,       "JumpIfNot" },
    { OpCode::Goto,            "Goto" },
    { OpCode::Return,          "Return" },
};

} // fraze

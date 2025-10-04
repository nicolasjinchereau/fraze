/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>

namespace fraze {

enum class OpCode : uint8_t
{
    // do nothing. Args will be printed to output for debugging
    NoOp,

    // Push a literal at DATA[arg] onto stack
    PushLiteral,

    // Push STACK[frameStart - paramCount - 1] onto stack
    PushContext,

    // Push STACK[bp + arg] onto stack
    PushLocal,
    PushLocalN,
    PushLocalAddr,

    // Pop stack into STACK[bp + arg]
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

    // context object is popped from the stack, and a field from that object is pushed to the stack
    PushField,
    PushFieldAddr,

    // context object and value are popped off stack and value is stored in object
    PopField,

    // context ref is popped from the stack, and a field from that object is pushed to the stack
    PushRefField,
    PushRefFieldN,
    PushRefFieldAddr,

    // context ref and value are popped off stack and value is stored in object
    PopRefField,

    // array is popped from the stack, and an element from that array is pushed to the stack
    PushElement,
    PushElementAddr,

    // the top element on the stack is stored in the array and the value and array are popped off stack
    PopElement,

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

    // push an array's element count to the stack
    PushCount,

    // push an array's total word size to the stack
    PushSize,

    // Pop stack and discard
    Pop,

    // Reserve space for return value, arg is word count
    Reserve,

    // push new array of #arg elements onto stack in place of the previous #arg elements
    NewArray,

    // push new instance of class [arg] onto stack
    NewClass,

    // replaces stack top with result of binary operation on top two (integers)
    LogicalOr,
    LogicalAnd,
    BitOr,
    BitXor,
    BitAnd,
    BitNot,
    LeftShift,
    RightShift,

    // replaces stack top with result of equality operation on top two (any Word, arg=size)
    Equal,
    EqualN,
    StringEqual,
    IsInstance,

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
    ConvBoolToStr,
    ConvIntToStr,
    ConvNumToStr,
    ConvEnumToStr,
    ConvObjToType,
    ConvRefToStruct,

    // arg is the WordType
    Box,

    // arg is the WordType
    Unbox,

    // concatenate two strings
    StringConcat,

    // pushes another copy of current stack top
    Dup,
    DupN,

    // calls function id on top of stack
    Call,

    // calls external function id on top of stack
    CallExternal,
    CallIntrinsic,

    // stack top is interface function id and arg is interface id
    CallVirtual,

    // jump to code[arg]
    Jump,

    // jump to code[arg] if stack[top] is true and pop stack
    JumpIf,

    // jump to code[arg] if stack[top] is false and pop stack
    JumpIfNot,

    // jump to stack[top]
    Goto,

    // Pop ip
    Return,

    // if stack[top - 1] is false, terminate with message at stack[top]
    Assert,

    COUNT, // number of enum members
};

inline std::unordered_map<OpCode, std::string> OpCodeNames {
    { OpCode::NoOp,            "NoOp" },
    { OpCode::PushLiteral,     "PushLiteral" },
    { OpCode::PushContext,     "PushContext" },
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
    { OpCode::PushElement,     "PushElement" },
    { OpCode::PushElementAddr, "PushElementAddr" },
    { OpCode::PopElement,      "PopElement" },
    { OpCode::PushField,       "PushField" },
    { OpCode::PushFieldAddr,   "PushFieldAddr" },
    { OpCode::PopField,        "PopField" },
    { OpCode::PushRefField,    "PushRefField" },
    { OpCode::PushRefFieldN,   "PushRefFieldN" },
    { OpCode::PushRefFieldAddr,"PushRefFieldAddr" },
    { OpCode::PopRefField,     "PopRefField" },
    { OpCode::PushBoolean,     "PushBoolean" },
    { OpCode::PushInteger,     "PushInteger" },
    { OpCode::PushNumber,      "PushNumber" },
    { OpCode::PushNull,        "PushNull" },
    { OpCode::PushCount,       "PushCount" },
    { OpCode::PushSize,        "PushSize" },
    { OpCode::Pop,             "Pop" },
    { OpCode::Reserve,         "Reserve" },
    { OpCode::NewArray,        "NewArray" },
    { OpCode::NewClass,        "NewClass" },

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
    { OpCode::StringEqual,     "StringEqual" },
    { OpCode::IsInstance,      "IsInstance" },

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
    { OpCode::ConvBoolToStr,   "ConvBoolToStr" },
    { OpCode::ConvIntToStr,    "ConvIntToStr" },
    { OpCode::ConvNumToStr,    "ConvNumToStr" },
    { OpCode::ConvEnumToStr,   "ConvEnumToStr" },
    { OpCode::ConvObjToType,   "ConvObjToType" },
    { OpCode::ConvRefToStruct, "ConvRefToStruct" },

    { OpCode::Box,             "Box" },
    { OpCode::Unbox,           "Unbox" },

    { OpCode::StringConcat,    "StringConcat" },

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
    { OpCode::Assert,          "Assert" },
};

} // fraze

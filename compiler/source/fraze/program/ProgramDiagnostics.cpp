/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/program/ProgramDiagnostics.h>
#include <fraze/program/Program.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

namespace fraze {

// Prints the program's literals and each function's operations to std::cout.
void ProgramDiagnostics::Print(const Program& program, bool printData, bool printCode)
{
    if(printData)
    {
        std::cout << "DATA:" << std::endl;

        // print data

        for(int i = 0; i != program.data.size(); ++i)
        {
            WordType type = program.dataTypes[i];
            std::cout << std::setw(4) << std::setfill('0') << i << ": "
                << WordTypeNames.at(type) << ", " << GetLiteralValue(program, i) << std::endl;
        }

        std::cout << std::endl;
    }

    if(printCode)
    {
        // print code
        for(auto& ti : program.typeInfo)
        {
            auto func = ti->ToFunctionInfo();
            if(!func)
                continue;

            std::cout << "FUNCTION: " << func->qualifiedName << std::endl;

            if(!func->externalFunction)
            {
                for(auto i = func->codeStart; i != func->codeEnd; ++i) {
                    PrintOperation(program, i, std::cout);
                    std::cout << std::endl;
                }
            }
            else
            {
                std::cout << "<external>" << std::endl;
            }

            std::cout << std::endl;
        }
    }
}

// Returns the literal at 'index' in the program's data, formatted as source.
std::string ProgramDiagnostics::GetLiteralValue(const Program& program, uint64_t index)
{
    Word value = program.data[index];
    WordType type = program.dataTypes[index];

    switch(type)
    {
    case WordType::Object:
        return "null";
    case WordType::Boolean:
        return value.GetBoolean() ? "true" : "false";
    case WordType::Integer:
        return std::to_string(value.GetInteger());
    case WordType::Number:
        return std::to_string(value.GetNumber());
    case WordType::String:
        return "\"" + std::string(value.GetString()->GetView()) + "\"";
    default:
        return "?";
    }
}

// Prints the operation at 'index' in the program's code: its index, its opcode name and its arguments.
void ProgramDiagnostics::PrintOperation(const Program& program, size_t index, std::ostream& stream)
{
    const Operation& op = program.code[index];
    assert(OpCodeNames.contains(op.code));

    stream << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << index << ": " << std::dec;

    switch(op.code)
    {
    case OpCode::Jump:
    case OpCode::JumpIf:
    case OpCode::JumpIfNot:
        stream << OpCodeNames[op.code] << ", "
            << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << op.arg1_u64 << std::dec;
        break;

    case OpCode::PushLiteral:
        stream << OpCodeNames[op.code] << ", "
            << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << op.arg1_u64
            << " [" << GetLiteralValue(program, op.arg1_u64) << "]" << std::dec;
        break;

    case OpCode::PushBoolean:
        stream << OpCodeNames[op.code] << ", " << std::boolalpha << (op.arg1_u64 != 0);
        break;

    case OpCode::PushNumber:
        stream << OpCodeNames[op.code] << ", " << static_cast<Number>(op.arg1_f64);
        break;

    //case OpCode::PushWord:
    //{
    //    stream << OpCodeNames[op.code] << ", " << op.arg1_u64
    //        << " [" << GetLiteralValue(program, op.arg1_u64) << "]" << std::dec;
    //    break;
    //}
    case OpCode::Call:
    case OpCode::CallExternal:
        stream << OpCodeNames[op.code] << ", " << program.typeInfo[op.arg1_u64]->qualifiedName;
        break;

    case OpCode::CallVirtual:
        stream << OpCodeNames[op.code] << ", " << program.typeInfo[op.arg1_u64]->qualifiedName;
        break;

    case OpCode::PushNull:
    case OpCode::Dup:
    case OpCode::Return:
    case OpCode::LogicalOr:
    case OpCode::LogicalAnd:
    case OpCode::BitOr:
    case OpCode::BitXor:
    case OpCode::BitAnd:
    case OpCode::LeftShift:
    case OpCode::RightShift:
    case OpCode::Equal:
    case OpCode::NotEqual:
    case OpCode::LessInt:
    case OpCode::LessNum:
    case OpCode::LessEqualInt:
    case OpCode::LessEqualNum:
    case OpCode::GreaterInt:
    case OpCode::GreaterNum:
    case OpCode::GreaterEqualInt:
    case OpCode::GreaterEqualNum:
    case OpCode::AddInt:
    case OpCode::AddNum:
    case OpCode::SubInt:
    case OpCode::SubNum:
    case OpCode::MulInt:
    case OpCode::MulNum:
    case OpCode::DivInt:
    case OpCode::DivNum:
    case OpCode::ModInt:
    case OpCode::ModNum:
    case OpCode::ConvIntToNum:
    case OpCode::ConvNumToInt:
        stream << OpCodeNames[op.code];
        break;

    case OpCode::PushLocalN:
    case OpCode::PopLocalN:
    case OpCode::PushGlobal:
    case OpCode::PopGlobal:
    case OpCode::PushArgumentN:
    case OpCode::PopArgument:
    case OpCode::PushWordN:
    case OpCode::PopWordN:
    case OpCode::PushIndexAddr:
        stream << OpCodeNames[op.code] << ", " << op.arg1_u64 << ", " << op.arg2_u64;
        break;

    case OpCode::PushInteger:
        stream << OpCodeNames[op.code] << ", " << op.arg1_i64;
        break;

    case OpCode::BitNot:
        stream << OpCodeNames[op.code] << ", " << op.arg1_i64;
        break;

    case OpCode::NoOp:
        stream << OpCodeNames[op.code];

        if(op.arg1_cstr)
            stream << ", " << op.arg1_cstr;

        if(op.arg2_cstr)
        {
            if(op.arg1_cstr)
                stream << ", ";

            stream << op.arg2_cstr;
        }
        break;

    case OpCode::EqualN:
    case OpCode::NotEqualN:
    case OpCode::PushLocal:
    case OpCode::PushLocalAddr:
    case OpCode::PopLocal:
    case OpCode::PushGlobalAddr:
    case OpCode::PushArgument:
    case OpCode::PushArgumentAddr:
    case OpCode::PushWord:
    case OpCode::PushWordAddr:
    case OpCode::PopWord:
    case OpCode::Pop:
    default:
        stream << OpCodeNames[op.code] << ", " << op.arg1_u64;
        break;
    }
}

// Prints the source line and column of the operation at 'index', then the operation, to std::cout.
void ProgramDiagnostics::PrintExecutedOperation(const Program& program, size_t index)
{
    const SourceLocation& loc = program.locations[index];
    std::cout << std::setw(4) << std::setfill(' ') << loc.line << ", ";
    std::cout << std::setw(3) << std::setfill(' ') << loc.column << ", ";
    PrintOperation(program, index, std::cout);
    std::cout << std::endl;
}

// Stores the time the operation about to execute starts, for EndOperationMeasurement.
void ProgramDiagnostics::BeginOperationMeasurement()
{
    operationStart = std::chrono::high_resolution_clock::now();
}

// Adds the time since BeginOperationMeasurement to the total for 'code', and counts one more execution of it.
void ProgramDiagnostics::EndOperationMeasurement(OpCode code)
{
    auto end = std::chrono::high_resolution_clock::now();
    auto codeIndex = static_cast<size_t>(code);
    opcodeTotalNanos[codeIndex] += duration_cast<std::chrono::nanoseconds>(end - operationStart).count();
    opcodeTotalCount[codeIndex] += 1;
}

// Prints each executed opcode's count, share of the total time and average time, slowest first.
void ProgramDiagnostics::DumpCodeProfile(std::ostream& stream)
{
    struct InstructionStats
    {
        OpCode code;
        uint64_t totalNanos;
        uint64_t totalCount;
        double nanosPerCall;
        double percentOfTotalTime;
    };

    std::vector<InstructionStats> counts;
    counts.reserve(static_cast<size_t>(OpCode::COUNT));
    uint64_t totalExecutionNanos = 0;

    for(size_t i = 0; i != static_cast<size_t>(OpCode::COUNT); ++i)
    {
        uint64_t totalCount = opcodeTotalCount[i];
        if(totalCount == 0)
            continue;

        OpCode code = static_cast<OpCode>(i);
        uint64_t totalNanos = opcodeTotalNanos[i];
        double nanosPerCall = static_cast<double>(totalNanos) / totalCount;

        totalExecutionNanos += totalNanos;

        counts.push_back(InstructionStats{ code, totalNanos, totalCount, nanosPerCall, 0.0 });
    }

    for(auto& item : counts) {
        item.percentOfTotalTime = static_cast<double>(item.totalNanos * 100) / totalExecutionNanos;
    }

    std::ranges::sort(counts, [](const InstructionStats& a, const InstructionStats& b){
        return a.totalNanos > b.totalNanos;
    });

    stream
        << std::left << std::setw(20) << "Code"
        << std::left << std::setw(16) << "Total Count"
        << std::left << std::setw(20) << "Fraction of Time"
        << std::left << "Nanos Per Call"
        << std::endl;

    for(auto& item : counts)
    {
        stream
            << std::left << std::setw(20) << OpCodeNames[item.code]
            << std::left << std::setw(16) << item.totalCount
            << std::left << std::setw(20) << std::fixed << std::setprecision(8) << item.percentOfTotalTime
            << std::left << std::fixed << std::setprecision(3) << item.nanosPerCall
            << std::endl;
    }

    stream << std::endl;
}

} // fraze

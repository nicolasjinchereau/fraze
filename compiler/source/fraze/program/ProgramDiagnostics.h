/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <fraze/program/OpCode.h>

namespace fraze {

class Program;

class ProgramDiagnostics
{
    static inline std::chrono::high_resolution_clock::time_point operationStart{};
    static inline std::array<uint64_t, static_cast<size_t>(OpCode::COUNT)> opcodeTotalNanos{};
    static inline std::array<uint64_t, static_cast<size_t>(OpCode::COUNT)> opcodeTotalCount{};

public:
    static void Print(const Program& program, bool printData = true, bool printCode = true);
    static void PrintOperation(const Program& program, size_t index, std::ostream& stream);
    static void PrintExecutedOperation(const Program& program, size_t index);
    static std::string GetLiteralValue(const Program& program, uint64_t index);

    static void BeginOperationMeasurement();
    static void EndOperationMeasurement(OpCode code);
    static void DumpCodeProfile(std::ostream& stream);
};

} // fraze

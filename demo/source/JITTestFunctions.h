/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>

namespace fraze {

class Compiler;
class Program;

// The externs demo/assets/jit/MainTest.fz calls, which exist to exercise the JIT's extern boundary.
void AddJITTestFunctions(Compiler& compiler);

Integer JITTest_Double(const Integer& value);
Number JITTest_Scale(Number value, Number factor);
Boolean JITTest_Not(const Boolean& value);
Boolean JITTest_RecordAndReturn(const Integer& value, const Boolean& result);
void JITTest_Record(const Integer& value);
Integer JITTest_GetRecorded();
Integer JITTest_Mix(bool flag, Integer value, Number amount);
Integer JITTest_AddWithProgram(Program* program, const Integer& left, const Integer& right);
void JITTest_CallBackIntoFraze(Program* program);

} // fraze

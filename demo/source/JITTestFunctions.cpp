/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <JITTestFunctions.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/NativeFunctions.h>
#include <fraze/program/Program.h>

namespace fraze {

static Integer recordedValue{};

void AddJITTestFunctions(Compiler& compiler)
{
    compiler.AddFunction<&JITTest_Double>("JITTest.Double");
    compiler.AddFunction<&JITTest_Scale>("JITTest.Scale");
    compiler.AddFunction<&JITTest_Not>("JITTest.Not");
    compiler.AddFunction<&JITTest_RecordAndReturn>("JITTest.RecordAndReturn");
    compiler.AddFunction<&JITTest_Record>("JITTest.Record");
    compiler.AddFunction<&JITTest_GetRecorded>("JITTest.GetRecorded");
    compiler.AddFunction<&JITTest_Mix>("JITTest.Mix");
    compiler.AddFunction<&JITTest_AddWithProgram>("JITTest.AddWithProgram");
    compiler.AddFunction<&JITTest_CallBackIntoFraze>("JITTest.CallBackIntoFraze");
    compiler.AddFunction<&Debug_Fail>("Debug.Fail");
    compiler.AddFunction<&String_Equals>("String.Equals");
}

Integer JITTest_Double(const Integer& value) {
    return value * 2;
}

Number JITTest_Scale(Number value, Number factor) {
    return value * factor;
}

Boolean JITTest_Not(const Boolean& value) {
    return !value;
}

Boolean JITTest_RecordAndReturn(const Integer& value, const Boolean& result) {
    recordedValue = value;
    return result;
}

void JITTest_Record(const Integer& value) {
    recordedValue = value;
}

Integer JITTest_GetRecorded() {
    return recordedValue;
}

Integer JITTest_Mix(bool flag, Integer value, Number amount) {
    return flag ? value + Integer(amount) : value - Integer(amount);
}

Integer JITTest_AddWithProgram(Program* program, const Integer& left, const Integer& right) {
    return program ? left + right : -1;
}

// Reaches compiled code from a native frame that compiled code called, so a failure inside it has two landing
// pads to cross on its way back to the host.
void JITTest_CallBackIntoFraze(Program* program) {
    program->Invoke("FailingAssert");
}

} // fraze

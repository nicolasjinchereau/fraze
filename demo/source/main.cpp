/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Exception.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/program/Dispatcher.h>
#include <ExternFunctions.h>
#include <iostream>
#include <print>
#include <string>

int main(int argc, char** argv)
{
    try
    {
        auto compiler = fraze::Compiler("../compiler/assets");

        compiler.AddDirectory("assets/scripts");
        AddExternFunctions(compiler);
        
#if !FRAZE_ASSERTS
        compiler.DisableAssert().DisableNullCheck().DisableBoundsCheck().DisableTypeCheck();
#endif

        auto program = compiler.Compile();

        auto dispatcher = fraze::Dispatcher::GetCurrent();
        
        dispatcher->InvokeAsync([&]{
            fraze::ScopedAllocator alloc(program.get());
            fraze::Array<fraze::String>* argArray = NEW_FRAZE_ARRAY_T(alloc, fraze::String, "string[]", 1);
            argArray->At(0) = NEW_FRAZE_STRING(alloc, "test");
            program->Invoke("main", argArray).GetInteger();
        });

        dispatcher->Run(true);

#if FRAZE_CODE_PROFILING
        program->DumpCodeProfile(std::cout);
#endif // FRAZE_CODE_PROFILING
    }
    catch(const std::exception& ex)
    {
        std::print("{}\n\n", ex.what());
    }

    return 0;
}

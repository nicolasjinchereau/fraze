/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Exception.h>
#include <fraze/common/ScopeUtil.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/program/Dispatcher.h>
#include <ExternFunctions.h>
#include <WorkerThread.h>
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
        //compiler.ExportLoweredCode("output/code");
#endif

        auto program = compiler.Compile();

        auto programRelease = fraze::scope_exit([]{
            fraze::WorkerThread::GetInstance().Shutdown();
            fraze::Dispatcher::GetCurrent()->Quit();
        });

        auto dispatcher = fraze::Dispatcher::GetCurrent();
        
        dispatcher->InvokeAsync([&]{
            fraze::ScopedAllocator alloc(program.get());
            fraze::Array<fraze::String>* argArray = NEW_FRAZE_ARRAY_T(alloc, fraze::String, "string[]", argc - 1);
            for(int i = 1; i < argc; ++i)
                argArray->At(i - 1) = NEW_FRAZE_STRING(alloc, argv[i]);
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
        return 1;
    }

    return 0;
}

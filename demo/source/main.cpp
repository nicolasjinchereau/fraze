/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Exception.h>
#include <fraze/common/ScopeUtil.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/program/Dispatcher.h>
#include <fraze/program/ProgramDiagnostics.h>
#include <ExternFunctions.h>
#include <WorkerThread.h>
#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
    bool useJIT = false;
    std::vector<std::string_view> frazeArgs;

    for(int i = 1; i < argc; ++i)
    {
        if(std::string_view(argv[i]) == "--jit")
            useJIT = true;
        else
            frazeArgs.push_back(argv[i]);
    }

    int exitCode = 0;

    try
    {
        fraze::Compiler compiler;
        compiler.AddFrazeRuntime("../compiler/assets");

        if(!useJIT)
        {
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
                fraze::Array<fraze::String>* argArray = NEW_FRAZE_ARRAY_T(alloc, fraze::String, "string[]", frazeArgs.size());
                for(size_t i = 0; i != frazeArgs.size(); ++i)
                    argArray->At(i) = NEW_FRAZE_STRING(alloc, frazeArgs[i]);
                program->Invoke("main", argArray).GetInteger();
            });

            dispatcher->Run(true);

#if FRAZE_CODE_PROFILING
            fraze::ProgramDiagnostics::DumpCodeProfile(std::cout);
#endif // FRAZE_CODE_PROFILING
        }
        else
        {
            compiler.SetCodeGenerator(fraze::CodeGenerator::JIT);
            compiler.AddFile("assets/jit/MainTest.fz");

            auto program = compiler.Compile();
            exitCode = static_cast<int>(program->Invoke("main").GetInteger());
        }
    }
    catch(const std::exception& ex)
    {
        std::print("{}\n\n", ex.what());
        exitCode = 1;
    }

    return exitCode;
}

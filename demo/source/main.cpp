/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
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

        auto program = compiler.Compile();

        auto dispatcher = fraze::Dispatcher::GetCurrent();
        
        dispatcher->InvokeAsync([&]{
            fraze::ScopedAllocator alloc(program.get());
            fraze::Array<fraze::String>* argArray = NEW_FRAZE_ARRAY(alloc, fraze::String, "string[]", 1);
            argArray->At(0) = NEW_FRAZE_STRING(alloc, "test");
            program->Invoke("main", argArray).GetInteger();
        });

        dispatcher->Run(true);
    }
    catch(const std::exception& ex)
    {
        std::print("{}\n\n", ex.what());
    }

    return 0;
}

/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <File.h>
#include <fstream>
#include <sstream>
#include <string>
#include <future>
#include <fraze/common/Exception.h>
#include <fraze/memory/ScopedAllocator.h>
#include <fraze/program/Program.h>
#include <fraze/program/Dispatcher.h>
#include <WindowsPlatform.h>
#include <WorkerThread.h>

namespace fraze {

String* File::ReadAllText(Program* program, const String& path)
{
    std::string pathStr { path.GetView() };

    std::ifstream file(pathStr, std::ios::binary);
    if(!file)
        Throw("Failed to open file: {}", pathStr);

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();

    if(size < 0)
        Throw("Failed to get size of file: {}", pathStr);

    ScopedAllocator allocator(program);
    String* text = NEW_FRAZE_STRING_N(allocator, static_cast<size_t>(size));

    file.seekg(0, std::ios::beg);
    file.read(text->GetChar(0), size);

    return text;
}

void File::ReadAllTextAsync(Program* program, Class& task, const String& path)
{
    Class* taskPtr = &task;
    program->PinMemory(taskPtr);

    std::string pathStr { path.GetView() };

    sptr<Dispatcher> dispatcher = Dispatcher::GetCurrent();

    WorkerThread::GetInstance().InvokeAsync([=]{
        std::ifstream file(pathStr, std::ios::binary);
        if(!file)
            Throw("Failed to open file: {}", pathStr);

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();

        if(size < 0)
            Throw("Failed to get size of file: {}", pathStr);

        ScopedAllocator allocator(program);
        String* text = NEW_FRAZE_STRING_N(allocator, static_cast<size_t>(size));

        file.seekg(0, std::ios::beg);
        file.read(text->GetChar(0), size);

        dispatcher->InvokeAsync([=]{
            taskPtr->SetField("$position", Integer(-1));
            taskPtr->SetField("$value", text);
            program->Invoke("OnAwaitableCompleted", taskPtr);
            program->UnpinMemory(taskPtr);
        });
    });
}

} // fraze

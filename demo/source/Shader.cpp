/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <future>
#include <Shader.h>
#include <Graphics.h>
#include <Texture.h>
#include <fraze/program/Program.h>
#include <fraze/program/Dispatcher.h>
#include <WorkerThread.h>

namespace fraze {

Shader::Shader(Graphics* graphics, std::string_view src, std::string_view vertexEntry, std::string_view pixelEntry)
{
    this->graphics = graphics;
    graphics->CreateShader(src, vertexEntry, pixelEntry, &resources);
}

Shader::~Shader()
{
}

void Shader::CreateShaderAsync(Program* program, Class& task, Graphics* graphics, std::string_view src, std::string_view vertexEntry, std::string_view pixelEntry)
{
    Class* taskPtr = &task;
    program->PinMemory(taskPtr);

    std::string sourceStr { src };
    std::string vertexEntryStr { vertexEntry };
    std::string pixelEntryStr { pixelEntry };

    sptr<Dispatcher> dispatcher = Dispatcher::GetCurrent();

    WorkerThread::GetInstance().InvokeAsync([=] {
        Shader* shader = new Shader(graphics, sourceStr, vertexEntryStr, pixelEntryStr);

        dispatcher->InvokeAsync([=]{
            taskPtr->SetField("$position", Integer(-1));
            taskPtr->SetField("$value", shader);
            program->Invoke("OnAwaitableCompleted", taskPtr);
            program->UnpinMemory(taskPtr);
        });
    });
}

WordType Shader::GetType() const {
    return WordType::Object;
}

void Shader::SetUniform(std::string_view name, const void* data, size_t size)
{
    auto it = resources.uniformInfo.find(name);
    if(it == resources.uniformInfo.end())
        return;

    if(size != it->second.size)
        throw Exception("data does not match uniform size");

    memcpy(resources.uniformData.data() + it->second.offset, data, it->second.size);
    uniformsChanged = true;
}

void Shader::SetUniform(std::string_view name, Texture* texture)
{
    assert(texture);

    auto it = resources.uniformInfo.find(name);
    if(it == resources.uniformInfo.end())
        return;

    if(it->second.size != 0)
        throw Exception("name does not identify a texture");

    graphics->SetTexture(texture->resourceView, texture->samplerState, it->second.offset);
}

bool Shader::HasUniform(std::string_view name) const {
    return resources.uniformInfo.contains(name);
}

} // fraze

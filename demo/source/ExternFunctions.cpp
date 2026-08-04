/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <ExternFunctions.h>
#include <Buffer.h>
#include <Graphics.h>
#include <Shader.h>
#include <Texture.h>
#include <Model.h>
#include <Window.h>
#include <Types.h>
#include <File.h>
#include <fraze/program/Program.h>
#include <fraze/compiler/Compiler.h>
#include <string>
#include <chrono>
#include <memory>

using clock_type = std::chrono::high_resolution_clock;

namespace fraze {

void AddExternFunctions(Compiler& compiler)
{
    compiler.AddFunction("RecordFrameStart", &RecordFrameStart);
    compiler.AddFunction("RecordFrameEnd", &RecordFrameEnd);
    compiler.AddFunction("Time.GetTicks", &Time_GetTicks);
    compiler.AddFunction("Time.GetTicksPerSecond", &Time_GetTicksPerSecond);
    compiler.AddFunction("NativeWindow.this", &NativeWindow_this);
    compiler.AddFunction("NativeWindow.Show", &NativeWindow_Show);
    compiler.AddFunction("NativeWindow.Hide", &NativeWindow_Hide);
    compiler.AddFunction("NativeWindow.Close", &NativeWindow_Close);
    compiler.AddFunction("NativeWindow.PumpMessage", &NativeWindow_PumpMessage);
    compiler.AddFunction("NativeWindow.GetWidth", &NativeWindow_GetWidth);
    compiler.AddFunction("NativeWindow.GetHeight", &NativeWindow_GetHeight);
    compiler.AddFunction("NativeGraphics.this", &NativeGraphics_this);
    compiler.AddFunction("NativeGraphics.SetRenderTarget", &NativeGraphics_SetRenderTarget);
    compiler.AddFunction("NativeGraphics.SetShader", &NativeGraphics_SetShader);
    compiler.AddFunction("NativeGraphics.SetVertexBuffer", &NativeGraphics_SetVertexBuffer);
    compiler.AddFunction("NativeGraphics.SetIndexBuffer", &NativeGraphics_SetIndexBuffer);
    compiler.AddFunction("NativeGraphics.SetClearColor", &NativeGraphics_SetClearColor);
    compiler.AddFunction("NativeGraphics.SetViewport", &NativeGraphics_SetViewport);
    compiler.AddFunction("NativeGraphics.GetViewport", &NativeGraphics_GetViewport);
    compiler.AddFunction("NativeGraphics.SetCullMode", &NativeGraphics_SetCullMode);
    compiler.AddFunction("NativeGraphics.SetScissorTestEnabled", &NativeGraphics_SetScissorTestEnabled);
    compiler.AddFunction("NativeGraphics.SetScissorRect", &NativeGraphics_SetScissorRect);
    compiler.AddFunction("NativeGraphics.SetDepthTest", &NativeGraphics_SetDepthTest);
    compiler.AddFunction("NativeGraphics.SetDepthWriteEnabled", &NativeGraphics_SetDepthWriteEnabled);
    compiler.AddFunction("NativeGraphics.SetBlendingEnabled", &NativeGraphics_SetBlendingEnabled);
    compiler.AddFunction("NativeGraphics.SetBlendOperations", &NativeGraphics_SetBlendOperations);
    compiler.AddFunction("NativeGraphics.SetBlendFactors", &NativeGraphics_SetBlendFactors);
    compiler.AddFunction("NativeGraphics.SetColorMask", &NativeGraphics_SetColorMask);
    compiler.AddFunction("NativeGraphics.SetBlendColor", &NativeGraphics_SetBlendColor);
    compiler.AddFunction("NativeGraphics.Clear", &NativeGraphics_Clear);
    compiler.AddFunction("NativeGraphics.Present", &NativeGraphics_Present);
    compiler.AddFunction("NativeGraphics.DrawArray", &NativeGraphics_DrawArray);
    compiler.AddFunction("NativeGraphics.DrawIndexed", &NativeGraphics_DrawIndexed);
    compiler.AddFunction("NativeShader.this", &NativeShader_this);
    compiler.AddFunction("NativeShader.SetUniformMat4", &NativeShader_SetUniformMat4);
    compiler.AddFunction("NativeShader.SetUniformTex", &NativeShader_SetUniformTex);
    compiler.AddFunction("Shader.CreateNativeShaderAsync", &Shader_CreateShaderObjectAsync);
    compiler.AddFunction("NativeTexture.this", &NativeTexture_this);
    compiler.AddFunction("Texture.CreateNativeTextureAsync", &Texture_CreateNativeTextureAsync);
    compiler.AddFunction("Model.ImportModel", &Model_ImportModel);
    compiler.AddFunction("Model.CreateSphereMesh", &Model_CreateSphereMesh);
    compiler.AddFunction("Model.ImportModelObjectAsync", &Model_ImportModelObjectAsync);
    compiler.AddFunction("NativeBuffer.this", "NativeBuffer(object,BufferType,BufferUsage,BufferCPUAccess,int)", &NativeBuffer_this_size);
    compiler.AddFunction("NativeBuffer.this", "NativeBuffer(object,BufferType,BufferUsage,BufferCPUAccess,void[])", &NativeBuffer_this_data);
    compiler.AddFunction("NativeBuffer.SetData", &NativeBuffer_SetData);
    compiler.AddFunction("NativeBuffer.GetSize", &NativeBuffer_GetSize);
    compiler.AddFunction("NativeBuffer.GetStride", &NativeBuffer_GetStride);
    compiler.AddFunction("File.ReadAllText", &File_ReadAllText);
    compiler.AddFunction("File.ReadAllTextAsync", &File_ReadAllTextAsync);
    compiler.AddIntrinsic("Mat4.operator+", "Mat4(Mat4,Mat4)", &Intrinsic_Mat4Add);
    compiler.AddIntrinsic("Mat4.operator-", "Mat4(Mat4,Mat4)", &Intrinsic_Mat4Sub);
    compiler.AddIntrinsic("Mat4.operator*", "Mat4(Mat4,Mat4)", &Intrinsic_Mat4Mul);
    compiler.AddIntrinsic("Mat4.operator*", "Mat4(Mat4,num)", &Intrinsic_Mat4NumMul);
    compiler.AddIntrinsic("Vec4.operator*", &Intrinsic_Vec4Mat4Mul);
}

//#define PRINT_FPS

#ifdef PRINT_FPS
clock_type::time_point _startTime;
uint64_t totalFrameNanos = 0;
uint64_t totalFrameCount = 0;

// MAIN
void RecordFrameStart() {
    _startTime = clock_type::now();
}

void RecordFrameEnd()
{
    auto end = clock_type::now();
    totalFrameNanos += duration_cast<std::chrono::nanoseconds>(end - _startTime).count();
    totalFrameCount += 1;

    if(totalFrameCount % 30 == 0)
    {
        double nanosPerFrame = static_cast<double>(totalFrameNanos) / totalFrameCount;
        double secondsPerFrame = nanosPerFrame / 1000000000.0;
        std::println("secondsPerFrame: {}", secondsPerFrame);
        std::println("FPS: {}", 1.0 / secondsPerFrame);
    }
}
#else
void RecordFrameStart(){}
void RecordFrameEnd(){}
#endif // PRINT_FPS

// TIME
Integer Time_GetTicks() {
    return static_cast<Integer>(clock_type::now().time_since_epoch().count());
}

Integer Time_GetTicksPerSecond() {
    static_assert(clock_type::period::den >= clock_type::period::num);
    constexpr uint64_t ticksPerSecond = clock_type::period::den / clock_type::period::num;
    return static_cast<Integer>(ticksPerSecond);
}

// WINDOW
Object* NativeWindow_this(Program* program, Object& window, const String& title, Integer x, Integer y, Integer width, Integer height) {
    ScopedAllocator allocator(program);
    return NEW_FRAZE_EXTERN_CLASS(allocator, Window, "NativeWindow",
        program,
        &window,
        title.GetView(),
        IVec2(static_cast<int>(x), static_cast<int>(y)),
        IVec2(static_cast<int>(width), static_cast<int>(height))
    );
}

void NativeWindow_Show(Object& self) {
    auto window = static_cast<Window*>(&self);
    window->Show();
}

void NativeWindow_Hide(Object& self) {
    auto window = static_cast<Window*>(&self);
    window->Hide();
}

void NativeWindow_Close(Object& self) {
    auto window = static_cast<Window*>(&self);
    window->Close();
}

Integer NativeWindow_GetWidth(Object& self) {
    auto window = static_cast<Window*>(&self);
    return window->GetSize().x;
}

Integer NativeWindow_GetHeight(Object& self) {
    auto window = static_cast<Window*>(&self);
    return window->GetSize().y;
}

Integer NativeWindow_PumpMessage(Object& self) {
    auto window = static_cast<Window*>(&self);
    return window->PumpMessage();
}

// GRAPHICS
Object* NativeGraphics_this(Program* program) {
    ScopedAllocator allocator(program);
    return NEW_FRAZE_EXTERN_CLASS(allocator, Graphics, "NativeGraphics");
}

void NativeGraphics_SetRenderTarget(Object& self, Object& windowObj) {
    auto graphics = static_cast<Graphics*>(&self);
    auto window = static_cast<Window*>(&windowObj);
    graphics->SetRenderTarget(window);
}

void NativeGraphics_SetShader(Object& self, Object& shaderObj) {
    auto graphics = static_cast<Graphics*>(&self);
    auto shader = static_cast<Shader*>(&shaderObj);
    graphics->SetShader(shader);
}

void NativeGraphics_SetVertexBuffer(Object& self, Object& bufferObj) {
    auto graphics = static_cast<Graphics*>(&self);
    auto buffer = static_cast<Buffer*>(&bufferObj);
    graphics->SetVertexBuffer(buffer);
}

void NativeGraphics_SetIndexBuffer(Object& self, Object& bufferObj) {
    auto graphics = static_cast<Graphics*>(&self);
    auto buffer = static_cast<Buffer*>(&bufferObj);
    graphics->SetIndexBuffer(buffer);
}

void NativeGraphics_SetClearColor(Object& self, const Color& color) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetClearColor(color);
}

void NativeGraphics_SetViewport(Object& self, IntRect rect) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetViewport(rect);
}

IntRect NativeGraphics_GetViewport(Object& self) {
    auto graphics = static_cast<Graphics*>(&self);
    return graphics->GetViewport();
}

void NativeGraphics_SetCullMode(Object& self, CullMode mode) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetCullMode(mode);
}

void NativeGraphics_SetScissorTestEnabled(Object& self, bool enabled) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetScissorTestEnabled(enabled);
}

void NativeGraphics_SetScissorRect(Object& self, IntRect rect) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetScissorRect(rect);
}

void NativeGraphics_SetDepthTest(Object& self, DepthTest test) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetDepthTest(test);
}

void NativeGraphics_SetDepthWriteEnabled(Object& self, bool enabled) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetDepthWriteEnabled(enabled);
}

void NativeGraphics_SetBlendingEnabled(Object& self, bool enabled) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetBlendingEnabled(enabled);
}

void NativeGraphics_SetBlendOperations(Object& self, BlendOperation colorBlendOp, BlendOperation alphaBlendOp) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetBlendOperations(colorBlendOp, alphaBlendOp);
}

void NativeGraphics_SetBlendFactors(Object& self, BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetBlendFactors(sourceColor, destColor, sourceAlpha, destAlpha);
}

void NativeGraphics_SetColorMask(Object& self, bool red, bool green, bool blue, bool alpha) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetColorMask(red, green, blue, alpha);
}

void NativeGraphics_SetBlendColor(Object& self, const Color& color) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->SetBlendColor(color);
}

void NativeGraphics_Clear(Object& self) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->Clear();
}

void NativeGraphics_Present(Object& self) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->Present();
}

void NativeGraphics_DrawArray(Object& self, Integer start, Integer count, DrawMode mode) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->DrawArray(static_cast<int>(start), static_cast<int>(count), mode);
}

void NativeGraphics_DrawIndexed(Object& self, Integer start, Integer count, DrawMode mode) {
    auto graphics = static_cast<Graphics*>(&self);
    graphics->DrawIndexed(static_cast<int>(start), static_cast<int>(count), mode);
}

// SHADER
Object* NativeShader_this(Program* program, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ScopedAllocator allocator(program);
    return NEW_FRAZE_EXTERN_CLASS(allocator, Shader, "NativeShader", graphics, src.GetView(), vertexEntry.GetView(), pixelEntry.GetView());
}

void NativeShader_SetUniformMat4(Object& self, const String& name, const Mat4& value) {
    auto shader = static_cast<Shader*>(&self);
    shader->SetUniform(name, value);
}

void NativeShader_SetUniformTex(Object& self, const String& name, Object& textureObj) {
    auto shader = static_cast<Shader*>(&self);
    auto texture = static_cast<Texture*>(&textureObj);
    shader->SetUniform(name, texture);
}

void Shader_CreateShaderObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry)
{
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    Shader::CreateShaderAsync(program, task, graphics, src, vertexEntry, pixelEntry);
}

// TEXTURE
Object* NativeTexture_this(Program* program, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ScopedAllocator allocator(program);
    return NEW_FRAZE_EXTERN_CLASS(allocator, Texture, "NativeTexture", graphics, path);
}

void Texture_CreateNativeTextureAsync(Program* program, Class& task, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    Texture::CreateTextureAsync(program, task, graphics, path);
}

// MODEL
Object* Model_ImportModel(Program* program, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ScopedAllocator allocator(program);
    return ModelImporter::ImportModel(allocator, graphics, std::string(path));
}

Object* Model_CreateSphereMesh(Program* program, Object& graphicsObj, Number radius, Integer segments, Integer rings, bool invert) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return ModelImporter::CreateSphereMesh(program, graphics, radius, segments, rings, invert);
}

void Model_ImportModelObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ModelImporter::ImportModelAsync(program, task, graphics, std::string(path));
}

// NATIVE BUFFER
Object* NativeBuffer_this_size(Program* program, Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, Integer size) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ScopedAllocator allocator(program);
    return NEW_FRAZE_EXTERN_CLASS(allocator, Buffer, "NativeBuffer", graphics, type, usage, cpuAccess, size);
}

Object* NativeBuffer_this_data(Program* program, Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ScopedAllocator allocator(program);
    return NEW_FRAZE_EXTERN_CLASS(allocator, Buffer, "NativeBuffer", graphics, type, usage, cpuAccess, data);
}

void NativeBuffer_SetData(Object& self, const Array<>& data)
{
    auto buffer = static_cast<Buffer*>(&self);
    buffer->SetData(data);
}

Integer NativeBuffer_GetSize(Object& self)
{
    auto buffer = static_cast<Buffer*>(&self);
    return static_cast<Integer>(buffer->GetSize());
}

Integer NativeBuffer_GetStride(Object& self)
{
    auto buffer = static_cast<Buffer*>(&self);
    return static_cast<Integer>(buffer->GetStride());
}

// FILE
String* File_ReadAllText(Program* program, const String& path)
{
    return File::ReadAllText(program, path);
}

void File_ReadAllTextAsync(Program* program, Class& task, const String& path)
{
    File::ReadAllTextAsync(program, task, path);
}

// MATH
void Intrinsic_Mat4Add(Word* rbp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = rbp - 2;
    Mat4& arg0 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0);
    Mat4& arg1 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0 - wordSizeArg1);
    Mat4* result = reinterpret_cast<Mat4*>(argsEnd - totalArgSize - wordSizeRet);

    result->m11 = arg0.m11 + arg1.m11;
    result->m12 = arg0.m12 + arg1.m12;
    result->m13 = arg0.m13 + arg1.m13;
    result->m14 = arg0.m14 + arg1.m14;
    result->m21 = arg0.m21 + arg1.m21;
    result->m22 = arg0.m22 + arg1.m22;
    result->m23 = arg0.m23 + arg1.m23;
    result->m24 = arg0.m24 + arg1.m24;
    result->m31 = arg0.m31 + arg1.m31;
    result->m32 = arg0.m32 + arg1.m32;
    result->m33 = arg0.m33 + arg1.m33;
    result->m34 = arg0.m34 + arg1.m34;
    result->m41 = arg0.m41 + arg1.m41;
    result->m42 = arg0.m42 + arg1.m42;
    result->m43 = arg0.m43 + arg1.m43;
    result->m44 = arg0.m44 + arg1.m44;
}

void Intrinsic_Mat4Sub(Word* rbp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = rbp - 2;
    Mat4& arg0 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0);
    Mat4& arg1 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0 - wordSizeArg1);
    Mat4* result = reinterpret_cast<Mat4*>(argsEnd - totalArgSize - wordSizeRet);

    result->m11 = arg0.m11 - arg1.m11;
    result->m12 = arg0.m12 - arg1.m12;
    result->m13 = arg0.m13 - arg1.m13;
    result->m14 = arg0.m14 - arg1.m14;
    result->m21 = arg0.m21 - arg1.m21;
    result->m22 = arg0.m22 - arg1.m22;
    result->m23 = arg0.m23 - arg1.m23;
    result->m24 = arg0.m24 - arg1.m24;
    result->m31 = arg0.m31 - arg1.m31;
    result->m32 = arg0.m32 - arg1.m32;
    result->m33 = arg0.m33 - arg1.m33;
    result->m34 = arg0.m34 - arg1.m34;
    result->m41 = arg0.m41 - arg1.m41;
    result->m42 = arg0.m42 - arg1.m42;
    result->m43 = arg0.m43 - arg1.m43;
    result->m44 = arg0.m44 - arg1.m44;
}

void Intrinsic_Mat4Mul(Word* rbp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = rbp - 2;
    Mat4& arg0 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0);
    Mat4& arg1 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0 - wordSizeArg1);
    Mat4* result = reinterpret_cast<Mat4*>(argsEnd - totalArgSize - wordSizeRet);

    result->m11 = arg0.m11 * arg1.m11 + arg0.m12 * arg1.m21 + arg0.m13 * arg1.m31 + arg0.m14 * arg1.m41;
    result->m12 = arg0.m11 * arg1.m12 + arg0.m12 * arg1.m22 + arg0.m13 * arg1.m32 + arg0.m14 * arg1.m42;
    result->m13 = arg0.m11 * arg1.m13 + arg0.m12 * arg1.m23 + arg0.m13 * arg1.m33 + arg0.m14 * arg1.m43;
    result->m14 = arg0.m11 * arg1.m14 + arg0.m12 * arg1.m24 + arg0.m13 * arg1.m34 + arg0.m14 * arg1.m44;

    result->m21 = arg0.m21 * arg1.m11 + arg0.m22 * arg1.m21 + arg0.m23 * arg1.m31 + arg0.m24 * arg1.m41;
    result->m22 = arg0.m21 * arg1.m12 + arg0.m22 * arg1.m22 + arg0.m23 * arg1.m32 + arg0.m24 * arg1.m42;
    result->m23 = arg0.m21 * arg1.m13 + arg0.m22 * arg1.m23 + arg0.m23 * arg1.m33 + arg0.m24 * arg1.m43;
    result->m24 = arg0.m21 * arg1.m14 + arg0.m22 * arg1.m24 + arg0.m23 * arg1.m34 + arg0.m24 * arg1.m44;

    result->m31 = arg0.m31 * arg1.m11 + arg0.m32 * arg1.m21 + arg0.m33 * arg1.m31 + arg0.m34 * arg1.m41;
    result->m32 = arg0.m31 * arg1.m12 + arg0.m32 * arg1.m22 + arg0.m33 * arg1.m32 + arg0.m34 * arg1.m42;
    result->m33 = arg0.m31 * arg1.m13 + arg0.m32 * arg1.m23 + arg0.m33 * arg1.m33 + arg0.m34 * arg1.m43;
    result->m34 = arg0.m31 * arg1.m14 + arg0.m32 * arg1.m24 + arg0.m33 * arg1.m34 + arg0.m34 * arg1.m44;

    result->m41 = arg0.m41 * arg1.m11 + arg0.m42 * arg1.m21 + arg0.m43 * arg1.m31 + arg0.m44 * arg1.m41;
    result->m42 = arg0.m41 * arg1.m12 + arg0.m42 * arg1.m22 + arg0.m43 * arg1.m32 + arg0.m44 * arg1.m42;
    result->m43 = arg0.m41 * arg1.m13 + arg0.m42 * arg1.m23 + arg0.m43 * arg1.m33 + arg0.m44 * arg1.m43;
    result->m44 = arg0.m41 * arg1.m14 + arg0.m42 * arg1.m24 + arg0.m43 * arg1.m34 + arg0.m44 * arg1.m44;
}

void Intrinsic_Mat4NumMul(Word* rbp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 1;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = rbp - 2;
    Mat4& arg0 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0);
    Number& arg1 = *reinterpret_cast<Number*>(argsEnd - wordSizeArg0 - wordSizeArg1);
    Mat4* result = reinterpret_cast<Mat4*>(argsEnd - totalArgSize - wordSizeRet);
    
    result->m11 = arg0.m11 * arg1;
    result->m12 = arg0.m12 * arg1;
    result->m13 = arg0.m13 * arg1;
    result->m14 = arg0.m14 * arg1;

    result->m21 = arg0.m21 * arg1;
    result->m22 = arg0.m22 * arg1;
    result->m23 = arg0.m23 * arg1;
    result->m24 = arg0.m24 * arg1;

    result->m31 = arg0.m31 * arg1;
    result->m32 = arg0.m32 * arg1;
    result->m33 = arg0.m33 * arg1;
    result->m34 = arg0.m34 * arg1;

    result->m41 = arg0.m41 * arg1;
    result->m42 = arg0.m42 * arg1;
    result->m43 = arg0.m43 * arg1;
    result->m44 = arg0.m44 * arg1;
}

void Intrinsic_Vec4Mat4Mul(Word* rbp)
{
    constexpr uint64_t wordSizeArg0 = 4;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 4;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;
    
    Word* argsEnd = rbp - 2;
    Vec4& arg0 = *reinterpret_cast<Vec4*>(argsEnd - wordSizeArg0);
    Mat4& arg1 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0 - wordSizeArg1);
    Vec4* result = reinterpret_cast<Vec4*>(argsEnd - totalArgSize - wordSizeRet);

    result->x = arg0.x * arg1.m11 + arg0.y * arg1.m21 + arg0.z * arg1.m31 + arg0.w * arg1.m41;
    result->y = arg0.x * arg1.m12 + arg0.y * arg1.m22 + arg0.z * arg1.m32 + arg0.w * arg1.m42;
    result->z = arg0.x * arg1.m13 + arg0.y * arg1.m23 + arg0.z * arg1.m33 + arg0.w * arg1.m43;
    result->w = arg0.x * arg1.m14 + arg0.y * arg1.m24 + arg0.z * arg1.m34 + arg0.w * arg1.m44;
}

} // fraze

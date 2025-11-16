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
#include <array>
#include <string>
#include <print>
#include <chrono>

using clock_type = std::chrono::high_resolution_clock;

namespace fraze {

void AddExternFunctions(Compiler& compiler)
{
    compiler.AddFunction("RecordFrameStart", &RecordFrameStart);
    compiler.AddFunction("RecordFrameEnd", &RecordFrameEnd);
    compiler.AddFunction("Time.GetTicks", &Time_GetTicks);
    compiler.AddFunction("Time.GetTicksPerSecond", &Time_GetTicksPerSecond);
    compiler.AddFunction("Window.CreateNativeWindow", &Window_CreateNativeWindow);
    compiler.AddFunction("Window.Show", &Window_Show);
    compiler.AddFunction("Window.Hide", &Window_Hide);
    compiler.AddFunction("Window.Close", &Window_Close);
    compiler.AddFunction("Window.PumpMessage", &Window_PumpMessage);
    compiler.AddFunction("Window.GetWidth", &Window_GetWidth);
    compiler.AddFunction("Window.GetHeight", &Window_GetHeight);
    compiler.AddFunction("Graphics.CreateNativeGraphics", &Graphics_CreateNativeGraphics);
    compiler.AddFunction("Graphics.SetRenderTarget", &Graphics_SetRenderTarget);
    compiler.AddFunction("Graphics.SetShader", &Graphics_SetShader);
    compiler.AddFunction("Graphics.SetVertexBuffer", &Graphics_SetVertexBuffer);
    compiler.AddFunction("Graphics.SetIndexBuffer", &Graphics_SetIndexBuffer);
    compiler.AddFunction("Graphics.SetClearColor", &Graphics_SetClearColor);
    compiler.AddFunction("Graphics.SetViewport", &Graphics_SetViewport);
    compiler.AddFunction("Graphics.GetViewport", &Graphics_GetViewport);
    compiler.AddFunction("Graphics.SetCullMode", &Graphics_SetCullMode);
    compiler.AddFunction("Graphics.SetScissorTestEnabled", &Graphics_SetScissorTestEnabled);
    compiler.AddFunction("Graphics.SetScissorRect", &Graphics_SetScissorRect);
    compiler.AddFunction("Graphics.SetDepthTest", &Graphics_SetDepthTest);
    compiler.AddFunction("Graphics.SetDepthWriteEnabled", &Graphics_SetDepthWriteEnabled);
    compiler.AddFunction("Graphics.SetBlendingEnabled", &Graphics_SetBlendingEnabled);
    compiler.AddFunction("Graphics.SetBlendOperations", &Graphics_SetBlendOperations);
    compiler.AddFunction("Graphics.SetBlendFactors", &Graphics_SetBlendFactors);
    compiler.AddFunction("Graphics.SetColorMask", &Graphics_SetColorMask);
    compiler.AddFunction("Graphics.SetBlendColor", &Graphics_SetBlendColor);
    compiler.AddFunction("Graphics.Clear", &Graphics_Clear);
    compiler.AddFunction("Graphics.Present", &Graphics_Present);
    compiler.AddFunction("Graphics.DrawArray", &Graphics_DrawArray);
    compiler.AddFunction("Graphics.DrawIndexed", &Graphics_DrawIndexed);
    compiler.AddFunction("Shader.CreateNativeShader", &Shader_CreateNativeShader);
    compiler.AddFunction("Shader.CreateNativeShaderAsync", &Shader_CreateShaderObjectAsync);
    compiler.AddFunction("Shader.SetUniformMat4", &Shader_SetUniformMat4);
    compiler.AddFunction("Shader.SetUniformTex", &Shader_SetUniformTex);
    compiler.AddFunction("Texture.CreateNativeTexture", &Texture_CreateNativeTexture);
    compiler.AddFunction("Texture.CreateNativeTextureAsync", &Texture_CreateNativeTextureAsync);
    compiler.AddFunction("Model.ImportModel", &Model_ImportModel);
    compiler.AddFunction("Model.CreateSphereMesh", &Model_CreateSphereMesh);
    compiler.AddFunction("Model.ImportModelObjectAsync", &Model_ImportModelObjectAsync);
    compiler.AddFunction("Buffer.CreateNativeBufferWithSize", &Buffer_CreateNativeBufferWithSize);
    compiler.AddFunction("Buffer.CreateNativeBufferFromData", &Buffer_CreateNativeBufferFromData);
    compiler.AddFunction("Buffer.SetData", &Buffer_SetData);
    compiler.AddFunction("Buffer.GetSize", &Buffer_GetSize);
    compiler.AddFunction("Buffer.GetStride", &Buffer_GetStride);
    compiler.AddFunction("File.ReadAllText", &File_ReadAllText);
    compiler.AddFunction("File.ReadAllTextAsync", &File_ReadAllTextAsync);
    compiler.AddIntrinsic("Mat4.operator+:Mat4(Mat4,Mat4)", &Intrinsic_Mat4Add);
    compiler.AddIntrinsic("Mat4.operator-:Mat4(Mat4,Mat4)", &Intrinsic_Mat4Sub);
    compiler.AddIntrinsic("Mat4.operator*:Mat4(Mat4,Mat4)", &Intrinsic_Mat4Mul);
    compiler.AddIntrinsic("Mat4.operator*:Mat4(Mat4,num)", &Intrinsic_Mat4NumMul);
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
Object* Window_CreateNativeWindow(Program* program, Object& window, const String& title, Integer x, Integer y, Integer width, Integer height) {
    return new Window(
        program,
        &window,
        title.GetView(),
        IVec2(static_cast<int>(x), static_cast<int>(y)),
        IVec2(static_cast<int>(width), static_cast<int>(height))
    );
}

void Window_Show(Object& windowObj) {
    auto window = static_cast<Window*>(&windowObj);
    window->Show();
}

void Window_Hide(Object& windowObj) {
    auto window = static_cast<Window*>(&windowObj);
    window->Hide();
}

void Window_Close(Object& windowObj) {
    auto window = static_cast<Window*>(&windowObj);
    window->Close();
}

Integer Window_GetWidth(Object& windowObj) {
    auto window = static_cast<Window*>(&windowObj);
    return window->GetSize().x;
}

Integer Window_GetHeight(Object& windowObj) {
    auto window = static_cast<Window*>(&windowObj);
    return window->GetSize().y;
}

Integer Window_PumpMessage(Object& windowObj) {
    auto window = static_cast<Window*>(&windowObj);
    return window->PumpMessage();
}

// GRAPHICS
Object* Graphics_CreateNativeGraphics() {
    return new Graphics();
}

void Graphics_SetRenderTarget(Object& graphicsObj, Object& windowObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    auto window = static_cast<Window*>(&windowObj);
    graphics->SetRenderTarget(window);
}

void Graphics_SetShader(Object& graphicsObj, Object& shaderObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    auto shader = static_cast<Shader*>(&shaderObj);
    graphics->SetShader(shader);
}

void Graphics_SetVertexBuffer(Object& graphicsObj, Object& bufferObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    auto buffer = static_cast<Buffer*>(&bufferObj);
    graphics->SetVertexBuffer(buffer);
}

void Graphics_SetIndexBuffer(Object& graphicsObj, Object& bufferObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    auto buffer = static_cast<Buffer*>(&bufferObj);
    graphics->SetIndexBuffer(buffer);
}

void Graphics_SetClearColor(Object& graphicsObj, const Color& color) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetClearColor(color);
}

void Graphics_SetViewport(Object& graphicsObj, IntRect rect) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetViewport(rect);
}

IntRect Graphics_GetViewport(Object& graphicsObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return graphics->GetViewport();
}

void Graphics_SetCullMode(Object& graphicsObj, CullMode mode) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetCullMode(mode);
}

void Graphics_SetScissorTestEnabled(Object& graphicsObj, bool enabled) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetScissorTestEnabled(enabled);
}

void Graphics_SetScissorRect(Object& graphicsObj, IntRect rect) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetScissorRect(rect);
}

void Graphics_SetDepthTest(Object& graphicsObj, DepthTest test) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetDepthTest(test);
}

void Graphics_SetDepthWriteEnabled(Object& graphicsObj, bool enabled) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetDepthWriteEnabled(enabled);
}

void Graphics_SetBlendingEnabled(Object& graphicsObj, bool enabled) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetBlendingEnabled(enabled);
}

void Graphics_SetBlendOperations(Object& graphicsObj, BlendOperation colorBlendOp, BlendOperation alphaBlendOp) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetBlendOperations(colorBlendOp, alphaBlendOp);
}

void Graphics_SetBlendFactors(Object& graphicsObj, BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetBlendFactors(sourceColor, destColor, sourceAlpha, destAlpha);
}

void Graphics_SetColorMask(Object& graphicsObj, bool red, bool green, bool blue, bool alpha) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetColorMask(red, green, blue, alpha);
}

void Graphics_SetBlendColor(Object& graphicsObj, const Color& color) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->SetBlendColor(color);
}

void Graphics_Clear(Object& graphicsObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->Clear();
}

void Graphics_Present(Object& graphicsObj) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->Present();
}

void Graphics_DrawArray(Object& graphicsObj, Integer start, Integer count, DrawMode mode) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->DrawArray(static_cast<int>(start), static_cast<int>(count), mode);
}

void Graphics_DrawIndexed(Object& graphicsObj, Integer start, Integer count, DrawMode mode) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    graphics->DrawIndexed(static_cast<int>(start), static_cast<int>(count), mode);
}

// SHADER
Object* Shader_CreateNativeShader(Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return new Shader(graphics, src.GetView(), vertexEntry.GetView(), pixelEntry.GetView());
}

void Shader_CreateShaderObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry)
{
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    Shader::CreateShaderAsync(program, task, graphics, src, vertexEntry, pixelEntry);
}

void Shader_SetUniformMat4(Object& shaderObj, const String& name, const Mat4& value) {
    auto shader = static_cast<Shader*>(&shaderObj);
    shader->SetUniform(name, value);
}

void Shader_SetUniformTex(Object& shaderObj, const String& name, Object& textureObj) {
    auto shader = static_cast<Shader*>(&shaderObj);
    auto texture = static_cast<Texture*>(&textureObj);
    shader->SetUniform(name, texture);
}

// TEXTURE
Object* Texture_CreateNativeTexture(Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return new Texture(graphics, path);
}

void Texture_CreateNativeTextureAsync(Program* program, Class& task, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    Texture::CreateTextureAsync(program, task, graphics, path);
}

// MODEL
Object* Model_ImportModel(Program* program, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return ModelImporter::ImportModel(program, graphics, std::string(path));
}

Object* Model_CreateSphereMesh(Program* program, Object& graphicsObj, Number radius, Integer segments, Integer rings, bool invert) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return ModelImporter::CreateSphereMesh(program, graphics, radius, segments, rings, invert);
}

void Model_ImportModelObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& path) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ModelImporter::ImportModelAsync(program, task, graphics, std::string(path));
}

// BUFFER
Object* Buffer_CreateNativeBufferWithSize(Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, Integer size) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return new Buffer(graphics, type, usage, cpuAccess, size);
}

Object* Buffer_CreateNativeBufferFromData(Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    return new Buffer(graphics, type, usage, cpuAccess, data);
}

void Buffer_SetData(Object& bufferObj, const Array<>& data)
{
    auto buffer = static_cast<Buffer*>(&bufferObj);
    buffer->SetData(data);
}

Integer Buffer_GetSize(Object& bufferObj)
{
    auto buffer = static_cast<Buffer*>(&bufferObj);
    return static_cast<Integer>(buffer->GetSize());
}

Integer Buffer_GetStride(Object& bufferObj)
{
    auto buffer = static_cast<Buffer*>(&bufferObj);
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
void Intrinsic_Mat4Add(const Operation& op, Word* stackTop, Word* bp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Mat4& arg0 = *reinterpret_cast<Mat4*>(bp - 2 - wordSizeArg0);
    Mat4& arg1 = *reinterpret_cast<Mat4*>(bp - 2 - wordSizeArg0 - wordSizeArg1);
    Mat4* result = reinterpret_cast<Mat4*>(bp - 2 - totalArgSize - wordSizeRet);

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

void Intrinsic_Mat4Sub(const Operation& op, Word* stackTop, Word* bp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = bp - 2;
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

void Intrinsic_Mat4Mul(const Operation& op, Word* stackTop, Word* bp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = bp - 2;
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

void Intrinsic_Mat4NumMul(const Operation& op, Word* stackTop, Word* bp)
{
    constexpr uint64_t wordSizeArg0 = 16;
    constexpr uint64_t wordSizeArg1 = 1;
    constexpr uint64_t wordSizeRet = 16;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;

    Word* argsEnd = bp - 2;
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

void Intrinsic_Vec4Mat4Mul(const Operation& op, Word* stackTop, Word* bp)
{
    constexpr uint64_t wordSizeArg0 = 4;
    constexpr uint64_t wordSizeArg1 = 16;
    constexpr uint64_t wordSizeRet = 4;
    constexpr uint64_t totalArgSize = wordSizeArg0 + wordSizeArg1;
    
    Word* argsEnd = bp - 2;
    Vec4& arg0 = *reinterpret_cast<Vec4*>(argsEnd - wordSizeArg0);
    Mat4& arg1 = *reinterpret_cast<Mat4*>(argsEnd - wordSizeArg0 - wordSizeArg1);
    Vec4* result = reinterpret_cast<Vec4*>(argsEnd - totalArgSize - wordSizeRet);

    result->x = arg0.x * arg1.m11 + arg0.y * arg1.m21 + arg0.z * arg1.m31 + arg0.w * arg1.m41;
    result->y = arg0.x * arg1.m12 + arg0.y * arg1.m22 + arg0.z * arg1.m32 + arg0.w * arg1.m42;
    result->z = arg0.x * arg1.m13 + arg0.y * arg1.m23 + arg0.z * arg1.m33 + arg0.w * arg1.m43;
    result->w = arg0.x * arg1.m14 + arg0.y * arg1.m24 + arg0.z * arg1.m34 + arg0.w * arg1.m44;
}

} // fraze

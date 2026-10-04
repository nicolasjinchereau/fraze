/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
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
#include <print>

using clock_type = std::chrono::high_resolution_clock;

namespace fraze {

void AddExternFunctions(Compiler& compiler)
{
    compiler.AddFunction<&RecordFrameStart>("RecordFrameStart");
    compiler.AddFunction<&RecordFrameEnd>("RecordFrameEnd");
    compiler.AddFunction<&Time_GetTicks>("Time.GetTicks");
    compiler.AddFunction<&Time_GetTicksPerSecond>("Time.GetTicksPerSecond");
    compiler.AddFunction<&NativeWindow_this>("NativeWindow.this");
    compiler.AddFunction<&NativeWindow_Show>("NativeWindow.Show");
    compiler.AddFunction<&NativeWindow_Hide>("NativeWindow.Hide");
    compiler.AddFunction<&NativeWindow_Close>("NativeWindow.Close");
    compiler.AddFunction<&NativeWindow_PumpMessage>("NativeWindow.PumpMessage");
    compiler.AddFunction<&NativeWindow_GetWidth>("NativeWindow.GetWidth");
    compiler.AddFunction<&NativeWindow_GetHeight>("NativeWindow.GetHeight");
    compiler.AddFunction<&NativeGraphics_this>("NativeGraphics.this");
    compiler.AddFunction<&NativeGraphics_SetRenderTarget>("NativeGraphics.SetRenderTarget");
    compiler.AddFunction<&NativeGraphics_SetShader>("NativeGraphics.SetShader");
    compiler.AddFunction<&NativeGraphics_SetVertexBuffer>("NativeGraphics.SetVertexBuffer");
    compiler.AddFunction<&NativeGraphics_SetIndexBuffer>("NativeGraphics.SetIndexBuffer");
    compiler.AddFunction<&NativeGraphics_SetClearColor>("NativeGraphics.SetClearColor");
    compiler.AddFunction<&NativeGraphics_SetViewport>("NativeGraphics.SetViewport");
    compiler.AddFunction<&NativeGraphics_GetViewport>("NativeGraphics.GetViewport");
    compiler.AddFunction<&NativeGraphics_SetCullMode>("NativeGraphics.SetCullMode");
    compiler.AddFunction<&NativeGraphics_SetScissorTestEnabled>("NativeGraphics.SetScissorTestEnabled");
    compiler.AddFunction<&NativeGraphics_SetScissorRect>("NativeGraphics.SetScissorRect");
    compiler.AddFunction<&NativeGraphics_SetDepthTest>("NativeGraphics.SetDepthTest");
    compiler.AddFunction<&NativeGraphics_SetDepthWriteEnabled>("NativeGraphics.SetDepthWriteEnabled");
    compiler.AddFunction<&NativeGraphics_SetBlendingEnabled>("NativeGraphics.SetBlendingEnabled");
    compiler.AddFunction<&NativeGraphics_SetBlendOperations>("NativeGraphics.SetBlendOperations");
    compiler.AddFunction<&NativeGraphics_SetBlendFactors>("NativeGraphics.SetBlendFactors");
    compiler.AddFunction<&NativeGraphics_SetColorMask>("NativeGraphics.SetColorMask");
    compiler.AddFunction<&NativeGraphics_SetBlendColor>("NativeGraphics.SetBlendColor");
    compiler.AddFunction<&NativeGraphics_Clear>("NativeGraphics.Clear");
    compiler.AddFunction<&NativeGraphics_Present>("NativeGraphics.Present");
    compiler.AddFunction<&NativeGraphics_DrawArray>("NativeGraphics.DrawArray");
    compiler.AddFunction<&NativeGraphics_DrawIndexed>("NativeGraphics.DrawIndexed");
    compiler.AddFunction<&NativeShader_this>("NativeShader.this");
    compiler.AddFunction<&NativeShader_SetUniformMat4>("NativeShader.SetUniformMat4");
    compiler.AddFunction<&NativeShader_SetUniformTex>("NativeShader.SetUniformTex");
    compiler.AddFunction<&Shader_CreateShaderObjectAsync>("Shader.CreateNativeShaderAsync");
    compiler.AddFunction<&NativeTexture_this>("NativeTexture.this");
    compiler.AddFunction<&Texture_CreateNativeTextureAsync>("Texture.CreateNativeTextureAsync");
    compiler.AddFunction<&Model_ImportModel>("Model.ImportModel");
    compiler.AddFunction<&Model_CreateSphereMesh>("Model.CreateSphereMesh");
    compiler.AddFunction<&Model_ImportModelObjectAsync>("Model.ImportModelObjectAsync");
    compiler.AddFunction<&NativeBuffer_this_size>("NativeBuffer.this", "NativeBuffer(object,BufferType,BufferUsage,BufferCPUAccess,int)");
    compiler.AddFunction<&NativeBuffer_this_data>("NativeBuffer.this", "NativeBuffer(object,BufferType,BufferUsage,BufferCPUAccess,void[])");
    compiler.AddFunction<&NativeBuffer_SetData>("NativeBuffer.SetData");
    compiler.AddFunction<&NativeBuffer_GetSize>("NativeBuffer.GetSize");
    compiler.AddFunction<&NativeBuffer_GetStride>("NativeBuffer.GetStride");
    compiler.AddFunction<&File_ReadAllText>("File.ReadAllText");
    compiler.AddFunction<&File_ReadAllTextAsync>("File.ReadAllTextAsync");
    compiler.AddFunction<&Mat4_Add>("Mat4.operator+", "Mat4(Mat4,Mat4)");
    compiler.AddFunction<&Mat4_Sub>("Mat4.operator-", "Mat4(Mat4,Mat4)");
    compiler.AddFunction<&Mat4_Mul>("Mat4.operator*", "Mat4(Mat4,Mat4)");
    compiler.AddFunction<&Mat4_NumMul>("Mat4.operator*", "Mat4(Mat4,num)");
    compiler.AddFunction<&Vec4_Mat4Mul>("Vec4.operator*");
}

//#define PRINT_FPS

#ifdef PRINT_FPS
std::array<uint64_t, 512> frameLengths{};
int nextFrame = 0;
clock_type::time_point _startTime;
clock_type::time_point _lastFpsPrint;

// MAIN
void RecordFrameStart() {
    _startTime = clock_type::now();
}

void RecordFrameEnd()
{
    auto now = clock_type::now();
    auto frameLength = (now - _startTime).count();
    frameLengths[nextFrame] = frameLength;
    nextFrame = (nextFrame + 1) % frameLengths.size();

    float timeSinceLastPrint = duration_cast<std::chrono::duration<float>>(now - _lastFpsPrint).count();
    if(timeSinceLastPrint > 1)
    {
        uint64_t totalFrameNanos = 0;
        for(auto nanos : frameLengths)
            totalFrameNanos += nanos;

        double nanosPerFrame = static_cast<double>(totalFrameNanos) / frameLengths.size();
        double secondsPerFrame = nanosPerFrame / 1000000000.0;
        double fps = 1.0 / secondsPerFrame;
        std::println("FPS: {}", fps);
        _lastFpsPrint = now;
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
    return allocator.NewExternClass<Window>("NativeWindow",
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
    return allocator.NewExternClass<Graphics>("NativeGraphics");
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
    return allocator.NewExternClass<Shader>("NativeShader", graphics, src.GetView(), vertexEntry.GetView(), pixelEntry.GetView());
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
    return allocator.NewExternClass<Texture>("NativeTexture", graphics, path);
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
    return allocator.NewExternClass<Buffer>("NativeBuffer", graphics, type, usage, cpuAccess, size);
}

Object* NativeBuffer_this_data(Program* program, Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data) {
    auto graphics = static_cast<Graphics*>(&graphicsObj);
    ScopedAllocator allocator(program);
    return allocator.NewExternClass<Buffer>("NativeBuffer", graphics, type, usage, cpuAccess, data);
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
Mat4 Mat4_Add(const Mat4& left, const Mat4& right)
{
    Mat4 result;

    result.m11 = left.m11 + right.m11;
    result.m12 = left.m12 + right.m12;
    result.m13 = left.m13 + right.m13;
    result.m14 = left.m14 + right.m14;
    result.m21 = left.m21 + right.m21;
    result.m22 = left.m22 + right.m22;
    result.m23 = left.m23 + right.m23;
    result.m24 = left.m24 + right.m24;
    result.m31 = left.m31 + right.m31;
    result.m32 = left.m32 + right.m32;
    result.m33 = left.m33 + right.m33;
    result.m34 = left.m34 + right.m34;
    result.m41 = left.m41 + right.m41;
    result.m42 = left.m42 + right.m42;
    result.m43 = left.m43 + right.m43;
    result.m44 = left.m44 + right.m44;
    return result;
}

Mat4 Mat4_Sub(const Mat4& left, const Mat4& right)
{
    Mat4 result;

    result.m11 = left.m11 - right.m11;
    result.m12 = left.m12 - right.m12;
    result.m13 = left.m13 - right.m13;
    result.m14 = left.m14 - right.m14;
    result.m21 = left.m21 - right.m21;
    result.m22 = left.m22 - right.m22;
    result.m23 = left.m23 - right.m23;
    result.m24 = left.m24 - right.m24;
    result.m31 = left.m31 - right.m31;
    result.m32 = left.m32 - right.m32;
    result.m33 = left.m33 - right.m33;
    result.m34 = left.m34 - right.m34;
    result.m41 = left.m41 - right.m41;
    result.m42 = left.m42 - right.m42;
    result.m43 = left.m43 - right.m43;
    result.m44 = left.m44 - right.m44;
    return result;
}

Mat4 Mat4_Mul(const Mat4& left, const Mat4& right)
{
    Mat4 result;

    result.m11 = left.m11 * right.m11 + left.m12 * right.m21 + left.m13 * right.m31 + left.m14 * right.m41;
    result.m12 = left.m11 * right.m12 + left.m12 * right.m22 + left.m13 * right.m32 + left.m14 * right.m42;
    result.m13 = left.m11 * right.m13 + left.m12 * right.m23 + left.m13 * right.m33 + left.m14 * right.m43;
    result.m14 = left.m11 * right.m14 + left.m12 * right.m24 + left.m13 * right.m34 + left.m14 * right.m44;

    result.m21 = left.m21 * right.m11 + left.m22 * right.m21 + left.m23 * right.m31 + left.m24 * right.m41;
    result.m22 = left.m21 * right.m12 + left.m22 * right.m22 + left.m23 * right.m32 + left.m24 * right.m42;
    result.m23 = left.m21 * right.m13 + left.m22 * right.m23 + left.m23 * right.m33 + left.m24 * right.m43;
    result.m24 = left.m21 * right.m14 + left.m22 * right.m24 + left.m23 * right.m34 + left.m24 * right.m44;

    result.m31 = left.m31 * right.m11 + left.m32 * right.m21 + left.m33 * right.m31 + left.m34 * right.m41;
    result.m32 = left.m31 * right.m12 + left.m32 * right.m22 + left.m33 * right.m32 + left.m34 * right.m42;
    result.m33 = left.m31 * right.m13 + left.m32 * right.m23 + left.m33 * right.m33 + left.m34 * right.m43;
    result.m34 = left.m31 * right.m14 + left.m32 * right.m24 + left.m33 * right.m34 + left.m34 * right.m44;

    result.m41 = left.m41 * right.m11 + left.m42 * right.m21 + left.m43 * right.m31 + left.m44 * right.m41;
    result.m42 = left.m41 * right.m12 + left.m42 * right.m22 + left.m43 * right.m32 + left.m44 * right.m42;
    result.m43 = left.m41 * right.m13 + left.m42 * right.m23 + left.m43 * right.m33 + left.m44 * right.m43;
    result.m44 = left.m41 * right.m14 + left.m42 * right.m24 + left.m43 * right.m34 + left.m44 * right.m44;

    return result;
}

Mat4 Mat4_NumMul(const Mat4& m, Number s)
{
    Mat4 result;
    
    result.m11 = m.m11 * s;
    result.m12 = m.m12 * s;
    result.m13 = m.m13 * s;
    result.m14 = m.m14 * s;

    result.m21 = m.m21 * s;
    result.m22 = m.m22 * s;
    result.m23 = m.m23 * s;
    result.m24 = m.m24 * s;

    result.m31 = m.m31 * s;
    result.m32 = m.m32 * s;
    result.m33 = m.m33 * s;
    result.m34 = m.m34 * s;

    result.m41 = m.m41 * s;
    result.m42 = m.m42 * s;
    result.m43 = m.m43 * s;
    result.m44 = m.m44 * s;

    return result;
}

Vec4 Vec4_Mat4Mul(const Vec4& v, const Mat4& m)
{
    Vec4 result;

    result.x = v.x * m.m11 + v.y * m.m21 + v.z * m.m31 + v.w * m.m41;
    result.y = v.x * m.m12 + v.y * m.m22 + v.z * m.m32 + v.w * m.m42;
    result.z = v.x * m.m13 + v.y * m.m23 + v.z * m.m33 + v.w * m.m43;
    result.w = v.x * m.m14 + v.y * m.m24 + v.z * m.m34 + v.w * m.m44;
    return result;
}

} // fraze

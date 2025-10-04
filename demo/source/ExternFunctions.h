/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <Types.h>

namespace fraze {

class Program;
struct Operation;
struct StackFrame;
class Compiler;

void AddExternFunctions(Compiler& compiler);

// MAIN
void RecordFrameStart();
void RecordFrameEnd();

// TIME
Integer Time_GetTicks();
Integer Time_GetTicksPerSecond();

// WINDOW
Object* Window_CreateNativeWindow(Program* program, Object& window, const String& title, Integer x, Integer y, Integer width, Integer height);
void Window_Show(Object& windowObj);
void Window_Hide(Object& windowObj);
void Window_Close(Object& windowObj);
Integer Window_PumpMessage(Object& windowObj);
void Window_Close(Object& windowObj);
Integer Window_GetWidth(Object& windowObj);
Integer Window_GetHeight(Object& windowObj);

// GRAPHICS
Object* Graphics_CreateNativeGraphics();
void Graphics_SetRenderTarget(Object& graphicsObj, Object& windowObj);
void Graphics_SetShader(Object& graphicsObj, Object& shaderObj);
void Graphics_SetVertexBuffer(Object& graphicsObj, Object& bufferObj);
void Graphics_SetIndexBuffer(Object& graphicsObj, Object& bufferObj);
void Graphics_SetClearColor(Object& graphicsObj, const Color& color);
void Graphics_SetViewport(Object& graphicsObj, IntRect rect);
IntRect Graphics_GetViewport(Object& graphicsObj);
void Graphics_SetCullMode(Object& graphicsObj, CullMode mode);
void Graphics_SetScissorTestEnabled(Object& graphicsObj, bool enabled);
void Graphics_SetScissorRect(Object& graphicsObj, IntRect rect);
void Graphics_SetDepthTest(Object& graphicsObj, DepthTest test);
void Graphics_SetDepthWriteEnabled(Object& graphicsObj, bool enabled);
void Graphics_SetBlendingEnabled(Object& graphicsObj, bool enabled);
void Graphics_SetBlendOperations(Object& graphicsObj, BlendOperation colorBlendOp, BlendOperation alphaBlendOp);
void Graphics_SetBlendFactors(Object& graphicsObj, BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha);
void Graphics_SetColorMask(Object& graphicsObj, bool red, bool green, bool blue, bool alpha);
void Graphics_SetBlendColor(Object& graphicsObj, const Color& color);
void Graphics_Clear(Object& graphicsObj);
void Graphics_Present(Object& graphicsObj);
void Graphics_DrawArray(Object& graphicsObj, Integer start, Integer count, DrawMode mode);
void Graphics_DrawIndexed(Object& graphicsObj, Integer start, Integer count, DrawMode mode);

// SHADER
Object* Shader_CreateNativeShader(Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry);
void Shader_CreateShaderObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry);
void Shader_SetUniformMat4(Object& shaderObj, const String& name, const Mat4& value);
void Shader_SetUniformTex(Object& shaderObj, const String& name, Object& textureObj);

// TEXTURE
Object* Texture_CreateNativeTexture(Object& graphicsObj, const String& path);
void Texture_CreateNativeTextureAsync(Program* program, Class& task, Object& graphicsObj, const String& path);

// MODEL
Object* Model_ImportModel(Program* program, Object& graphicsObj, const String& path);
Object* Model_CreateSphereMesh(Program* program, Object& graphicsObj, Number radius, Integer segments, Integer rings, bool invert);
void Model_ImportModelObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& path);

// BUFFER
Object* Buffer_CreateNativeBufferWithSize(Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, Integer size);
Object* Buffer_CreateNativeBufferFromData(Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data);
void Buffer_SetData(Object& bufferObj, const Array<>& data);
Integer Buffer_GetSize(Object& bufferObj);
Integer Buffer_GetStride(Object& bufferObj);

// FILE
String* File_ReadAllText(Program* program, const String& path);
void File_ReadAllTextAsync(Program* program, Class& task, const String& path);

// MATH
void Intrinsic_Mat4Add(const Operation& op, Word*& RESTRICT stackTop, StackFrame* RESTRICT fp);
void Intrinsic_Mat4Sub(const Operation& op, Word*& RESTRICT stackTop, StackFrame* RESTRICT fp);
void Intrinsic_Mat4Mul(const Operation& op, Word*& RESTRICT stackTop, StackFrame* RESTRICT fp);
void Intrinsic_Mat4NumMul(const Operation& op, Word*& RESTRICT stackTop, StackFrame* RESTRICT fp);
void Intrinsic_Vec4Mat4Mul(const Operation& op, Word*& RESTRICT stackTop, StackFrame* RESTRICT fp);


} // fraze

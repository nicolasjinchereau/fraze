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
Object* NativeWindow_this(Program* program, Object& window, const String& title, Integer x, Integer y, Integer width, Integer height);
void NativeWindow_Show(Object& windowObj);
void NativeWindow_Hide(Object& windowObj);
void NativeWindow_Close(Object& windowObj);
Integer NativeWindow_PumpMessage(Object& windowObj);
Integer NativeWindow_GetWidth(Object& windowObj);
Integer NativeWindow_GetHeight(Object& windowObj);

// GRAPHICS
Object* NativeGraphics_this(Program* program);
void NativeGraphics_SetRenderTarget(Object& graphicsObj, Object& windowObj);
void NativeGraphics_SetShader(Object& graphicsObj, Object& shaderObj);
void NativeGraphics_SetVertexBuffer(Object& graphicsObj, Object& bufferObj);
void NativeGraphics_SetIndexBuffer(Object& graphicsObj, Object& bufferObj);
void NativeGraphics_SetClearColor(Object& graphicsObj, const Color& color);
void NativeGraphics_SetViewport(Object& graphicsObj, IntRect rect);
IntRect NativeGraphics_GetViewport(Object& graphicsObj);
void NativeGraphics_SetCullMode(Object& graphicsObj, CullMode mode);
void NativeGraphics_SetScissorTestEnabled(Object& graphicsObj, bool enabled);
void NativeGraphics_SetScissorRect(Object& graphicsObj, IntRect rect);
void NativeGraphics_SetDepthTest(Object& graphicsObj, DepthTest test);
void NativeGraphics_SetDepthWriteEnabled(Object& graphicsObj, bool enabled);
void NativeGraphics_SetBlendingEnabled(Object& graphicsObj, bool enabled);
void NativeGraphics_SetBlendOperations(Object& graphicsObj, BlendOperation colorBlendOp, BlendOperation alphaBlendOp);
void NativeGraphics_SetBlendFactors(Object& graphicsObj, BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha);
void NativeGraphics_SetColorMask(Object& graphicsObj, bool red, bool green, bool blue, bool alpha);
void NativeGraphics_SetBlendColor(Object& graphicsObj, const Color& color);
void NativeGraphics_Clear(Object& graphicsObj);
void NativeGraphics_Present(Object& graphicsObj);
void NativeGraphics_DrawArray(Object& graphicsObj, Integer start, Integer count, DrawMode mode);
void NativeGraphics_DrawIndexed(Object& graphicsObj, Integer start, Integer count, DrawMode mode);

// SHADER
Object* NativeShader_this(Program* program, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry);
void NativeShader_SetUniformMat4(Object& shaderObj, const String& name, const Mat4& value);
void NativeShader_SetUniformTex(Object& shaderObj, const String& name, Object& textureObj);
void Shader_CreateShaderObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry);

// TEXTURE
Object* NativeTexture_this(Program* program, Object& graphicsObj, const String& path);
void Texture_CreateNativeTextureAsync(Program* program, Class& task, Object& graphicsObj, const String& path);

// MODEL
Object* Model_ImportModel(Program* program, Object& graphicsObj, const String& path);
Object* Model_CreateSphereMesh(Program* program, Object& graphicsObj, Number radius, Integer segments, Integer rings, bool invert);
void Model_ImportModelObjectAsync(Program* program, Class& task, Object& graphicsObj, const String& path);

// NATIVE BUFFER
Object* NativeBuffer_this_size(Program* program, Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, Integer size);
Object* NativeBuffer_this_data(Program* program, Object& graphicsObj, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data);
void NativeBuffer_SetData(Object& bufferObj, const Array<>& data);
Integer NativeBuffer_GetSize(Object& bufferObj);
Integer NativeBuffer_GetStride(Object& bufferObj);

// FILE
String* File_ReadAllText(Program* program, const String& path);
void File_ReadAllTextAsync(Program* program, Class& task, const String& path);

// MATH
void Intrinsic_Mat4Add(Word* rbp);
void Intrinsic_Mat4Sub(Word* rbp);
void Intrinsic_Mat4Mul(Word* rbp);
void Intrinsic_Mat4NumMul(Word* rbp);
void Intrinsic_Vec4Mat4Mul(Word* rbp);

} // fraze

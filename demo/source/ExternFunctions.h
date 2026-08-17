/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
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
void NativeWindow_Show(Object& self);
void NativeWindow_Hide(Object& self);
void NativeWindow_Close(Object& self);
Integer NativeWindow_PumpMessage(Object& self);
Integer NativeWindow_GetWidth(Object& self);
Integer NativeWindow_GetHeight(Object& self);

// GRAPHICS
Object* NativeGraphics_this(Program* program);
void NativeGraphics_SetRenderTarget(Object& self, Object& windowObj);
void NativeGraphics_SetShader(Object& self, Object& shaderObj);
void NativeGraphics_SetVertexBuffer(Object& self, Object& bufferObj);
void NativeGraphics_SetIndexBuffer(Object& self, Object& bufferObj);
void NativeGraphics_SetClearColor(Object& self, const Color& color);
void NativeGraphics_SetViewport(Object& self, IntRect rect);
IntRect NativeGraphics_GetViewport(Object& self);
void NativeGraphics_SetCullMode(Object& self, CullMode mode);
void NativeGraphics_SetScissorTestEnabled(Object& self, bool enabled);
void NativeGraphics_SetScissorRect(Object& self, IntRect rect);
void NativeGraphics_SetDepthTest(Object& self, DepthTest test);
void NativeGraphics_SetDepthWriteEnabled(Object& self, bool enabled);
void NativeGraphics_SetBlendingEnabled(Object& self, bool enabled);
void NativeGraphics_SetBlendOperations(Object& self, BlendOperation colorBlendOp, BlendOperation alphaBlendOp);
void NativeGraphics_SetBlendFactors(Object& self, BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha);
void NativeGraphics_SetColorMask(Object& self, bool red, bool green, bool blue, bool alpha);
void NativeGraphics_SetBlendColor(Object& self, const Color& color);
void NativeGraphics_Clear(Object& self);
void NativeGraphics_Present(Object& self);
void NativeGraphics_DrawArray(Object& self, Integer start, Integer count, DrawMode mode);
void NativeGraphics_DrawIndexed(Object& self, Integer start, Integer count, DrawMode mode);

// SHADER
Object* NativeShader_this(Program* program, Object& graphicsObj, const String& src, const String& vertexEntry, const String& pixelEntry);
void NativeShader_SetUniformMat4(Object& self, const String& name, const Mat4& value);
void NativeShader_SetUniformTex(Object& self, const String& name, Object& textureObj);
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
void NativeBuffer_SetData(Object& self, const Array<>& data);
Integer NativeBuffer_GetSize(Object& self);
Integer NativeBuffer_GetStride(Object& self);

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

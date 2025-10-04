/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Exception.h>
#include <fraze/common/Object.h>
#include <Types.h>
#include <WindowsPlatform.h>
#include <array>
#include <string>
#include <mutex>

namespace fraze {

class Buffer;
class Shader;
class Texture;
class Window;
struct ShaderResources;
struct WindowResources;

class Graphics : public Object
{
    D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_11_1;
    ComPtr<ID3D11Device1> device;
    ComPtr<ID3D11DeviceContext1> context;

    // blend
    ComPtr<ID3D11BlendState> blendState;
    D3D11_RENDER_TARGET_BLEND_DESC targetBlendDesc {
        .BlendEnable = false,
        .SrcBlend = D3D11_BLEND_ONE,
        .DestBlend = D3D11_BLEND_ZERO,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_ZERO,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_DEPTH_WRITE_MASK_ALL
    };

    Color blendColor = { 1.0, 1.0, 1.0, 1.0 };
    bool blendStateDirty = true;

    // rasterizer
    ComPtr<ID3D11RasterizerState> rasterizerState;
    D3D11_CULL_MODE cullMode = D3D11_CULL_BACK;
    bool scissorTestEnabled = false;
    bool rasterizerStateDirty = true;

    // depth stencil
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    bool depthTestEnabled = true;
    D3D11_DEPTH_WRITE_MASK depthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    D3D11_COMPARISON_FUNC depthFunc = D3D11_COMPARISON_LESS_EQUAL;
    bool depthStencilStateDirty = true;

    // other
    int swapInterval = 0;
    Color clearColor { 0.0, 0.0, 0.0, 0.0 };
    IntRect viewport{};

    Window* target{};
    Shader* shader{};
    Buffer* vertexBuffer{};
    Buffer* indexBuffer{};

    std::mutex mut;
public:

    Graphics();
    ~Graphics();

    virtual WordType GetType() const override;

    void SetRenderTarget(Window* window);
    void SetShader(Shader* shader);
    void SetVertexBuffer(Buffer* buffer);
    void SetIndexBuffer(Buffer* buffer);
    void SetClearColor(const Color& color);
    void SetViewport(IntRect rect);
    IntRect GetViewport();
    void SetCullMode(CullMode mode);
    void SetScissorTestEnabled(bool enabled);
    void SetScissorRect(IntRect rect);
    void SetDepthTest(DepthTest test);
    void SetDepthWriteEnabled(bool enabled);
    void SetBlendingEnabled(bool enabled);
    void SetBlendOperations(BlendOperation colorBlendOp, BlendOperation alphaBlendOp);
    void SetBlendFactors(BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha);
    void SetColorMask(bool red, bool green, bool blue, bool alpha);
    void SetBlendColor(Color color);
    void Clear(bool color = true, bool depth = true);
    void Present();
    void DrawArray(int start, int count, DrawMode mode);
    void DrawIndexed(int start, int count, DrawMode mode);

    ComPtr<ID3D11Buffer> CreateBuffer(BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const void* data, size_t size);
    void* MapBuffer(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource, BufferMapAccess mapAccess);
    void UnmapBuffer(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource);
    void CreateTexture(int width, int height, PixelDataFormat format, const std::vector<std::byte>& data, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppResourceView);
    void CreateSamplerState(TextureFilterMode filterMode, TextureWrapMode wrapMode, ID3D11SamplerState** ppSamplerState);
    void SetTexture(const ComPtr<ID3D11ShaderResourceView>& resourceView, const ComPtr<ID3D11SamplerState>& samplerState, uint32_t offset);
    void CreateShader(std::string_view src, std::string_view vertexEntry, std::string_view pixelEntry, ShaderResources* pResources);
    void ResizeSurface(uint32_t width, uint32_t height, WindowResources* pResources);

private:
    void UpdateDeviceStates();
    void UpdateShader();
    void SetRenderTargetInternal(Window* window);
    void SetViewportInternal(IntRect rect);
    void SetScissorRectInternal(IntRect rect);
    void* MapBufferInternal(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource, BufferMapAccess mapAccess);
    void UnmapBufferInternal(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource);
    ComPtr<ID3DBlob> CompileShader(std::string_view source, const std::string& entryPoint, const std::string& profile);
    void GetShaderInfo(const ComPtr<ID3DBlob>& vsBlob, const ComPtr<ID3DBlob>& psBlob, ShaderResources* pResources);
    void UpdateBufferInternal(const ComPtr<ID3D11Buffer>& buffer, const std::vector<std::byte>& data);
    void CreateSurfaceInternal(Window* window);
    void DestroySurfaceInternal(Window* window);
};

} // fraze

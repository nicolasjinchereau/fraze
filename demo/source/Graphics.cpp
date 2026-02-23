/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <Graphics.h>
#include <Shader.h>
#include <Buffer.h>
#include <Window.h>

namespace fraze {

inline namespace d3d11 {

std::unordered_map<BlendFactor, D3D11_BLEND> blendFactors = {
    { BlendFactor::Zero, D3D11_BLEND_ZERO },
    { BlendFactor::One, D3D11_BLEND_ONE },
    { BlendFactor::SrcColor, D3D11_BLEND_SRC_COLOR },
    { BlendFactor::OneMinusSrcColor, D3D11_BLEND_INV_SRC_COLOR },
    { BlendFactor::DstColor, D3D11_BLEND_DEST_COLOR },
    { BlendFactor::OneMinusDstColor, D3D11_BLEND_INV_DEST_COLOR },
    { BlendFactor::SrcAlpha, D3D11_BLEND_SRC_ALPHA },
    { BlendFactor::OneMinusSrcAlpha, D3D11_BLEND_INV_SRC_ALPHA },
    { BlendFactor::DstAlpha, D3D11_BLEND_DEST_ALPHA },
    { BlendFactor::OneMinusDstAlpha, D3D11_BLEND_INV_DEST_ALPHA },
    { BlendFactor::SrcAlphaSaturate, D3D11_BLEND_SRC_ALPHA_SAT },
    { BlendFactor::ConstColor, D3D11_BLEND_BLEND_FACTOR },
    { BlendFactor::OneMinusConstColor, D3D11_BLEND_INV_BLEND_FACTOR },
    { BlendFactor::Src1Color, D3D11_BLEND_SRC1_COLOR },
    { BlendFactor::OneMinusSrc1Color, D3D11_BLEND_INV_SRC1_COLOR },
    { BlendFactor::Src1Alpha, D3D11_BLEND_SRC1_ALPHA },
    { BlendFactor::OneMinusSrc1Alpha, D3D11_BLEND_INV_SRC1_ALPHA }
};

std::unordered_map<BlendOperation, D3D11_BLEND_OP> blendOperations = {
    { BlendOperation::Add, D3D11_BLEND_OP_ADD },
    { BlendOperation::Subtract, D3D11_BLEND_OP_SUBTRACT },
    { BlendOperation::ReverseSubtract, D3D11_BLEND_OP_REV_SUBTRACT },
    { BlendOperation::Min, D3D11_BLEND_OP_MIN },
    { BlendOperation::Max, D3D11_BLEND_OP_MAX }
};

std::unordered_map<DrawMode, D3D11_PRIMITIVE_TOPOLOGY> primitiveTopologies = {
    { DrawMode::Points, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST },
    { DrawMode::Lines, D3D11_PRIMITIVE_TOPOLOGY_LINELIST },
    { DrawMode::LineStrip, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP },
    { DrawMode::Triangles, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST },
    { DrawMode::TriangleStrip, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP }
};

std::unordered_map<CullMode, D3D11_CULL_MODE> cullModes = {
    { CullMode::Off, D3D11_CULL_NONE },
    { CullMode::Back, D3D11_CULL_BACK },
    { CullMode::Front, D3D11_CULL_FRONT },
};

std::unordered_map<DepthTest, std::pair<bool, D3D11_COMPARISON_FUNC>> depthFuncs = {
    { DepthTest::Always, { false, D3D11_COMPARISON_ALWAYS } },
    { DepthTest::Never, { true, D3D11_COMPARISON_NEVER } },
    { DepthTest::Less, { true, D3D11_COMPARISON_LESS } },
    { DepthTest::Equal, { true, D3D11_COMPARISON_EQUAL } },
    { DepthTest::LessOrEqual, { true, D3D11_COMPARISON_LESS_EQUAL } },
    { DepthTest::Greater, { true, D3D11_COMPARISON_GREATER } },
    { DepthTest::NotEqual, { true, D3D11_COMPARISON_NOT_EQUAL } },
    { DepthTest::GreaterOrEqual, { true, D3D11_COMPARISON_GREATER_EQUAL } },
};

std::unordered_map<BufferType, D3D11_BIND_FLAG> bufferTypes = {
    { BufferType::Vertex, D3D11_BIND_VERTEX_BUFFER },
    { BufferType::Index, D3D11_BIND_INDEX_BUFFER },
};

std::unordered_map<BufferUsage, D3D11_USAGE> bufferUsage = {
    { BufferUsage::Default, D3D11_USAGE_DEFAULT },
    { BufferUsage::Dynamic, D3D11_USAGE_DYNAMIC },
    { BufferUsage::Static, D3D11_USAGE_IMMUTABLE },
    { BufferUsage::Stream, D3D11_USAGE_STAGING }
};

std::unordered_map<BufferCPUAccess, UINT> bufferCPUAccess =
{
    { BufferCPUAccess::None, 0 },
    { BufferCPUAccess::ReadOnly, D3D11_CPU_ACCESS_READ },
    { BufferCPUAccess::WriteOnly, D3D11_CPU_ACCESS_WRITE },
    { BufferCPUAccess::ReadWrite, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE }
};

std::unordered_map<BufferMapAccess, D3D11_MAP> bufferMapAccess = {
    { BufferMapAccess::ReadOnly, D3D11_MAP_READ },
    { BufferMapAccess::WriteOnly, D3D11_MAP_WRITE },
    { BufferMapAccess::ReadWrite, D3D11_MAP_READ_WRITE },
    { BufferMapAccess::WriteDiscard, D3D11_MAP_WRITE_DISCARD },
    { BufferMapAccess::WriteNoSync, D3D11_MAP_WRITE_DISCARD }
};

std::unordered_map<TextureWrapMode, D3D11_TEXTURE_ADDRESS_MODE> wrapModes = {
    { TextureWrapMode::Clamp, D3D11_TEXTURE_ADDRESS_CLAMP },
    { TextureWrapMode::Repeat, D3D11_TEXTURE_ADDRESS_WRAP }
};

std::unordered_map<PixelDataFormat, DXGI_FORMAT> textureFormats = {
    { PixelDataFormat::Alpha8, DXGI_FORMAT_A8_UNORM },
    { PixelDataFormat::RGB24, DXGI_FORMAT_R8G8B8A8_UNORM },
    { PixelDataFormat::RGBA32, DXGI_FORMAT_R8G8B8A8_UNORM },
    { PixelDataFormat::RGBAFloat, DXGI_FORMAT_R32G32B32A32_FLOAT }
};

std::unordered_map<PixelDataFormat, int> textureComponents = {
    { PixelDataFormat::Alpha8, 1 },
    { PixelDataFormat::RGB24, 4 },
    { PixelDataFormat::RGBA32, 4 },
    { PixelDataFormat::RGBAFloat, 4 }
};

std::unordered_map<TextureFilterMode, D3D11_FILTER> filterModes = {
    { TextureFilterMode::Point, D3D11_FILTER_MIN_MAG_MIP_POINT },
    { TextureFilterMode::Bilinear, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT },
    { TextureFilterMode::Trilinear, D3D11_FILTER_MIN_MAG_MIP_LINEAR }
};

} // d3d11

Graphics::Graphics(const TypeInfo* typeInfo)
    : Object(typeInfo)
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    //creationFlags &= ~D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifndef NDEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };

    ComPtr<ID3D11Device> tmpDevice;
    ComPtr<ID3D11DeviceContext> tmpContext;

    HRESULT res;

    res = D3D11CreateDevice(nullptr,
        D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
        featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
        &tmpDevice, &mFeatureLevel, &tmpContext
    );
    if(FAILED(res))
        throw Exception("failed to create d3d11 device");

    res = tmpDevice.As(&device);
    if(FAILED(res))
        throw Exception("failed to get d3d11 device interface");

    res = tmpContext.As(&context);
    if(FAILED(res))
        throw Exception("failed to get d3d11 device context interface");

    blendStateDirty = true;
    rasterizerStateDirty = true;
    depthStencilStateDirty = true;

    SetColorMask(true, true, true, true);
    UpdateDeviceStates();
}

Graphics::~Graphics()
{
    std::lock_guard<std::mutex> lk(mut);
    blendState = nullptr;
    rasterizerState = nullptr;
    depthStencilState = nullptr;
    context = nullptr;
    device = nullptr;
}

WordType Graphics::GetType() const {
    return WordType::Object;
}

void Graphics::SetRenderTarget(Window* window)
{
    std::lock_guard<std::mutex> lk(mut);
    SetRenderTargetInternal(window);
}

void Graphics::SetShader(Shader* shader)
{
    std::lock_guard<std::mutex> lk(mut);

    this->shader = shader;

    if(shader)
    {
        context->IASetInputLayout(shader->resources.inputLayout.Get());
        context->VSSetShader(shader->resources.vertexShader.Get(), nullptr, 0);
        context->PSSetShader(shader->resources.pixelShader.Get(), nullptr, 0);
        context->VSSetConstantBuffers(0, 1, shader->resources.uniformBuffer.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, shader->resources.uniformBuffer.GetAddressOf());
    }
    else
    {
        context->IASetInputLayout(nullptr);
        context->VSSetShader(nullptr, nullptr, 0);
        context->PSSetShader(nullptr, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, nullptr);
        context->PSSetConstantBuffers(0, 1, nullptr);
    }
}

void Graphics::SetVertexBuffer(Buffer* buffer)
{
    std::lock_guard<std::mutex> lk(mut);

    ENFORCE(buffer->type == BufferType::Vertex, SourceLocation(), "not a vertex buffer");
    this->vertexBuffer = buffer;

    if(buffer)
    {
        UINT stride  = static_cast<UINT>(buffer->GetNativeStride());
        UINT offset  = 0;
        context->IASetVertexBuffers(0, 1, buffer->buffer.GetAddressOf(), &stride, &offset);
    }
    else
    {
        UINT zero = 0;
        ID3D11Buffer* nullBuf[1] = { nullptr };
        context->IASetVertexBuffers(0, 1, nullBuf, &zero, &zero);
    }
}

void Graphics::SetIndexBuffer(Buffer* buffer)
{
    std::lock_guard<std::mutex> lk(mut);

    ENFORCE(buffer->type == BufferType::Index, SourceLocation(), "not a vertex buffer");
    this->indexBuffer = buffer;

    if(buffer)
    {
        context->IASetIndexBuffer(buffer->buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    }
    else
    {
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    }
}

void Graphics::SetClearColor(const Color& color)
{
    std::lock_guard<std::mutex> lk(mut);
    clearColor = color;
}

void Graphics::SetViewport(IntRect rect)
{
    std::lock_guard<std::mutex> lk(mut);
    SetViewportInternal(rect);
}

IntRect Graphics::GetViewport()
{
    std::lock_guard<std::mutex> lk(mut);
    return viewport;
}

void Graphics::SetCullMode(CullMode mode)
{
    std::lock_guard<std::mutex> lk(mut);
    cullMode = cullModes[mode];
    rasterizerStateDirty = true;
}

void Graphics::SetScissorTestEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lk(mut);
    scissorTestEnabled = enabled;
    rasterizerStateDirty = true;
}

void Graphics::SetScissorRect(IntRect rect)
{
    std::lock_guard<std::mutex> lk(mut);
    SetScissorRectInternal(rect);
}

void Graphics::SetDepthTest(DepthTest test)
{
    std::lock_guard<std::mutex> lk(mut);
    auto& [enabled, func] = depthFuncs[test];
    depthTestEnabled = enabled;
    depthFunc = func;
    depthStencilStateDirty = true;
}

void Graphics::SetDepthWriteEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lk(mut);
    depthWriteMask = enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilStateDirty = true;
}

void Graphics::SetBlendingEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lk(mut);
    targetBlendDesc.BlendEnable = enabled;
    blendStateDirty = true;
}

void Graphics::SetBlendOperations(BlendOperation colorBlendOp, BlendOperation alphaBlendOp)
{
    std::lock_guard<std::mutex> lk(mut);
    targetBlendDesc.BlendOp = blendOperations[colorBlendOp];
    targetBlendDesc.BlendOpAlpha = blendOperations[alphaBlendOp];
    blendStateDirty = true;
}

void Graphics::SetBlendFactors(BlendFactor sourceColor, BlendFactor destColor, BlendFactor sourceAlpha, BlendFactor destAlpha)
{
    std::lock_guard<std::mutex> lk(mut);
    targetBlendDesc.SrcBlend = blendFactors[sourceColor];
    targetBlendDesc.DestBlend = blendFactors[destColor];
    targetBlendDesc.SrcBlendAlpha = blendFactors[sourceAlpha];
    targetBlendDesc.DestBlendAlpha = blendFactors[destAlpha];
    blendStateDirty = true;
}

void Graphics::SetColorMask(bool red, bool green, bool blue, bool alpha)
{
    std::lock_guard<std::mutex> lk(mut);
    UINT8 writeMask = 0;
    if(red)   writeMask |= D3D11_COLOR_WRITE_ENABLE_RED;
    if(green) writeMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    if(blue)  writeMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    if(alpha) writeMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    targetBlendDesc.RenderTargetWriteMask = writeMask;
    blendStateDirty = true;
}

void Graphics::SetBlendColor(Color color)
{
    std::lock_guard<std::mutex> lk(mut);
    blendColor = color;
    assert(blendState.Get());
    auto data = AsFloatArray(blendColor);
    context->OMSetBlendState(blendState.Get(), data.data(), 0xFFFFFFFF);
}

void Graphics::Clear(bool color, bool depth)
{
    std::lock_guard<std::mutex> lk(mut);

    if(!target)
        return;

    if(color)
    {
        auto data = AsFloatArray(clearColor);
        context->ClearRenderTargetView(target->resources.renderTargetView.Get(), data.data());
    }

    if(depth) {
        context->ClearDepthStencilView(target->resources.depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

void Graphics::Present()
{
    std::lock_guard<std::mutex> lk(mut);

    if(!target)
        return;

    target->resources.swapChain->Present(swapInterval, 0);
    SetRenderTargetInternal(target);
}

void Graphics::DrawArray(int start, int count, DrawMode mode)
{
    std::lock_guard<std::mutex> lk(mut);
    UpdateShader();
    UpdateDeviceStates();
    context->IASetPrimitiveTopology(primitiveTopologies[mode]);
    context->Draw(count, start);
}

void Graphics::DrawIndexed(int start, int count, DrawMode mode)
{
    std::lock_guard<std::mutex> lk(mut);
    UpdateShader();
    UpdateDeviceStates();
    context->IASetPrimitiveTopology(primitiveTopologies[mode]);
    context->DrawIndexed(count, start, 0);
}

ComPtr<ID3D11Buffer> Graphics::CreateBuffer(BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const void* data, size_t size)
{
    std::lock_guard<std::mutex> lk(mut);

    ComPtr<ID3D11Buffer> buffer;

    auto typ = bufferTypes[type];
    auto usg = bufferUsage[usage];
    auto acc = bufferCPUAccess[cpuAccess];

    D3D11_SUBRESOURCE_DATA bufferData;
    bufferData.pSysMem = data;
    bufferData.SysMemPitch = 0;
    bufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC bufferDesc((UINT)size, typ, usg, acc);

    HRESULT res = device->CreateBuffer(&bufferDesc, &bufferData, &buffer);
    if(FAILED(res))
        Throw("could not create buffer");

    return buffer;
}

void* Graphics::MapBuffer(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource, BufferMapAccess mapAccess)
{
    std::lock_guard<std::mutex> lk(mut);
    return MapBufferInternal(buffer, subresource, mapAccess);
}

void Graphics::UnmapBuffer(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource)
{
    std::lock_guard<std::mutex> lk(mut);
    UnmapBufferInternal(buffer, subresource);
}

bool IsPowerOfTwo(unsigned int num) {
    return (num != 0) && ((num & (num - 1)) == 0);
}

void Graphics::CreateTexture(int width, int height, PixelDataFormat format, const std::vector<std::byte>& data, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppResourceView)
{
    std::lock_guard<std::mutex> lk(mut);

    bool generateMipmaps = false;

    if(IsPowerOfTwo(width) && IsPowerOfTwo(height))
    {
        UINT fmtSupport = 0;
        
        auto result = device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &fmtSupport);
        if(SUCCEEDED(result) && (fmtSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN))
        {
            generateMipmaps = true;
        }
    }

    UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
    UINT miscFlags = 0;
    UINT mipLevels = 1;
    UINT resourceViewMipLevels = 1;

    bindFlags |= D3D11_BIND_RENDER_TARGET;

    if(generateMipmaps)
    {
        miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        mipLevels = 0; // generate all levels
        resourceViewMipLevels = -1; // generate all levels
    }

    D3D11_TEXTURE2D_DESC textureDesc;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = mipLevels;
    textureDesc.ArraySize = 1;
    textureDesc.Format = textureFormats[format];
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = bindFlags;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = miscFlags;

    D3D11_SUBRESOURCE_DATA subResData;
    subResData.pSysMem = data.data();
    subResData.SysMemPitch = width * textureComponents[format];
    subResData.SysMemSlicePitch = width * height * textureComponents[format];

    D3D11_SUBRESOURCE_DATA* pSubResData = generateMipmaps ? nullptr : &subResData;

    HRESULT res = device->CreateTexture2D(&textureDesc, pSubResData, ppTexture);
    if(FAILED(res))
        throw Exception("texture creation failed");

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResViewDesc;
    shaderResViewDesc.Format = textureDesc.Format;
    shaderResViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResViewDesc.Texture2D.MipLevels = resourceViewMipLevels;

    res = device->CreateShaderResourceView(*ppTexture, &shaderResViewDesc, ppResourceView);
    if(FAILED(res))
        throw Exception("texture creation failed");

    if(generateMipmaps)
    {
        context->UpdateSubresource(
            *ppTexture, 0, nullptr,
            data.data(), subResData.SysMemPitch, subResData.SysMemSlicePitch);

        context->GenerateMips(*ppResourceView);
    }
}

void Graphics::CreateSamplerState(TextureFilterMode filterMode, TextureWrapMode wrapMode, ID3D11SamplerState** ppSamplerState)
{
    std::lock_guard<std::mutex> lk(mut);

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = filterModes[filterMode];
    samplerDesc.AddressU = wrapModes[wrapMode];
    samplerDesc.AddressV = wrapModes[wrapMode];
    samplerDesc.AddressW = wrapModes[wrapMode];
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    auto res = device->CreateSamplerState(&samplerDesc, ppSamplerState);
    if (FAILED(res))
        throw Exception("could not create sampler state for texture");
}

void Graphics::SetTexture(const ComPtr<ID3D11ShaderResourceView>& resourceView, const ComPtr<ID3D11SamplerState>& samplerState, uint32_t offset)
{
    std::lock_guard<std::mutex> lk(mut);

    ID3D11ShaderResourceView* resourceViews[1] = {
        resourceView ? resourceView.Get() : nullptr
    };

    ID3D11SamplerState* samplerStates[1] = {
        samplerState ? samplerState.Get() : nullptr
    };

    context->VSSetShaderResources(offset, 1, resourceViews); 
    context->VSSetSamplers(offset, 1, samplerStates);

    context->PSSetShaderResources(offset, 1, resourceViews);
    context->PSSetSamplers(offset, 1, samplerStates);
}

void Graphics::CreateShader(std::string_view src, std::string_view vertexEntry, std::string_view pixelEntry, ShaderResources* pResources)
{
    std::lock_guard<std::mutex> lk(mut);

    auto vsBytecode = CompileShader(std::string(src), std::string(vertexEntry), "vs_5_0");
    auto psBytecode = CompileShader(std::string(src), std::string(pixelEntry), "ps_5_0");

    HRESULT hr;

    hr = device->CreateVertexShader(
        vsBytecode->GetBufferPointer(),
        vsBytecode->GetBufferSize(),
        nullptr,
        &pResources->vertexShader);

    if(FAILED(hr))
        throw Exception("could not create vertex shader");

    hr = device->CreatePixelShader(
        psBytecode->GetBufferPointer(),
        psBytecode->GetBufferSize(),
        nullptr,
        &pResources->pixelShader);

    if(FAILED(hr))
        throw Exception("could not create pixel shader");

    GetShaderInfo(vsBytecode, psBytecode, pResources);
}

void Graphics::ResizeSurface(uint32_t width, uint32_t height, WindowResources* pResources)
{
    std::lock_guard<std::mutex> lk(mut);

    ID3D11RenderTargetView* nullRTV = nullptr;
    context->OMSetRenderTargets(1, &nullRTV, nullptr);

    pResources->renderTargetView = nullptr;
    pResources->depthStencilView = nullptr;

    HRESULT res;
    res = pResources->swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if(FAILED(res))
        Throw("ResizeBuffers failed");

    // create render target view
    ComPtr<ID3D11Texture2D> backBufferTex;
    pResources->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTex);
    res = device->CreateRenderTargetView(backBufferTex.Get(), nullptr, &pResources->renderTargetView);

    if(FAILED(res))
        Throw("failed to create d3d11 render target view");

    // create depth stencil view
    CD3D11_TEXTURE2D_DESC depthTextureDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, width, height, 1, 1, D3D11_BIND_DEPTH_STENCIL);
    ComPtr<ID3D11Texture2D> depthStencilTex;
    res = device->CreateTexture2D(&depthTextureDesc, nullptr, &depthStencilTex);

    if(FAILED(res))
        Throw("failed to create texture for d3d11 depth stencil");

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    res = device->CreateDepthStencilView(
        depthStencilTex.Get(), &depthStencilViewDesc, &pResources->depthStencilView);

    if(FAILED(res))
        Throw("failed to create d3d11 depth stencil view");

    context->OMSetRenderTargets(1, pResources->renderTargetView.GetAddressOf(), pResources->depthStencilView.Get());

    SetViewportInternal({ 0, 0, width, height });
    SetScissorRectInternal({ 0, 0, width, height });
}

// PRIVATE

void Graphics::UpdateDeviceStates()
{
    if (blendStateDirty)
    {
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0] = targetBlendDesc;

        auto res = device->CreateBlendState(&blendDesc, &blendState);
        if(FAILED(res))
            throw Exception("failed to create d3d11 blend state");

        auto data = AsFloatArray(blendColor);
        context->OMSetBlendState(blendState.Get(), data.data(), 0xFFFFFFFF);
        blendStateDirty = false;
    }

    if (rasterizerStateDirty)
    {
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.AntialiasedLineEnable = false;
        rasterDesc.CullMode = cullMode;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.DepthClipEnable = true;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.FrontCounterClockwise = false;
        rasterDesc.MultisampleEnable = false;
        rasterDesc.ScissorEnable = scissorTestEnabled;
        rasterDesc.SlopeScaledDepthBias = 0.0f;

        auto res = device->CreateRasterizerState(&rasterDesc, &rasterizerState);
        if(FAILED(res))
            throw Exception("failed to create d3d11 raster state");

        context->RSSetState(rasterizerState.Get());
        rasterizerStateDirty = false;
    }

    if (depthStencilStateDirty)
    {
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = depthTestEnabled;
        depthStencilDesc.DepthWriteMask = depthWriteMask;
        depthStencilDesc.DepthFunc = depthFunc;
        depthStencilDesc.StencilEnable = false;
        depthStencilDesc.StencilReadMask = 0xFF;
        depthStencilDesc.StencilWriteMask = 0xFF;
        depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        auto res = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
        if(FAILED(res))
            throw Exception("failed to create d3d11 depth stencil state");

        context->OMSetDepthStencilState(depthStencilState.Get(), 1);
        depthStencilStateDirty = false;
    }
}

void Graphics::UpdateShader()
{
    if(shader && shader->uniformsChanged)
    {
        UpdateBufferInternal(shader->resources.uniformBuffer, shader->resources.uniformData);
    }
}

void Graphics::SetRenderTargetInternal(Window* window)
{
    if(window)
    {
        if(window->graphics != this)
        {
            DestroySurfaceInternal(window);
            CreateSurfaceInternal(window);
            window->graphics = this;
        }

        context->OMSetRenderTargets(1, window->resources.renderTargetView.GetAddressOf(), window->resources.depthStencilView.Get());

        IntRect rc = { 0, 0, window->size.x, window->size.y };
        SetViewportInternal(rc);
        SetScissorRectInternal(rc);
    }
    else
    {
        context->OMSetRenderTargets(0, nullptr, nullptr);
    }

    target = window;
}

void Graphics::SetViewportInternal(IntRect rect)
{
    viewport = rect;
    CD3D11_VIEWPORT viewport((float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h);
    context->RSSetViewports(1, &viewport);
}

void Graphics::SetScissorRectInternal(IntRect rect)
{
    D3D11_RECT rc;
    rc.left = (LONG)rect.x;
    rc.top = (LONG)rect.x;
    rc.right = (LONG)(rect.x + rect.w);
    rc.bottom = (LONG)(rect.y + rect.h);
    context->RSSetScissorRects(1, &rc);
}

void* Graphics::MapBufferInternal(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource, BufferMapAccess mapAccess)
{
    auto access = bufferMapAccess[mapAccess];

    D3D11_MAPPED_SUBRESOURCE resource;
    HRESULT res = context->Map(buffer.Get(), 0, access, 0, &resource);
    if(FAILED(res))
        Throw("Failed to map buffer: {}", GetLastError());

    return resource.pData;
}

void Graphics::UnmapBufferInternal(const ComPtr<ID3D11Buffer>& buffer, uint32_t subresource)
{
    context->Unmap(buffer.Get(), subresource);
}

ComPtr<ID3DBlob> Graphics::CompileShader(std::string_view source, const std::string& entryPoint, const std::string& profile)
{
    ComPtr<ID3DBlob> result;
    ComPtr<ID3DBlob> error;

    UINT compileFlags = 0;
    compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
    //compileFlags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    //compileFlags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

    HRESULT hr = D3DCompile(
        source.data(),
        source.size(),
        NULL, NULL, NULL,
        entryPoint.c_str(),
        profile.c_str(),
        compileFlags, 0,
        &result,
        &error);

    if(FAILED(hr)) {
        std::string err = (const char*)error->GetBufferPointer();
        throw Exception(err);
    }

    return result;
}

void Graphics::GetShaderInfo(const ComPtr<ID3DBlob>& vsBlob, const ComPtr<ID3DBlob>& psBlob, ShaderResources* pResources)
{
    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        IID_ID3D11ShaderReflection, (void**)&reflector);

    D3D11_SHADER_DESC shaderDesc;
    reflector->GetDesc(&shaderDesc);

    // create input layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> layoutDescs;

    for(UINT i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        reflector->GetInputParameterDesc(i, &paramDesc);

        D3D11_INPUT_ELEMENT_DESC desc = {};
        desc.SemanticName = paramDesc.SemanticName;
        desc.SemanticIndex = paramDesc.SemanticIndex;
        desc.InputSlot = 0;
        desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        desc.InstanceDataStepRate = 0;

        // Map component type + mask to DXGI format
        if(paramDesc.Mask == 1)
        {
            if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                desc.Format = DXGI_FORMAT_R32_UINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                desc.Format = DXGI_FORMAT_R32_SINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                desc.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if(paramDesc.Mask <= 3)
        {
            if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                desc.Format = DXGI_FORMAT_R32G32_UINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                desc.Format = DXGI_FORMAT_R32G32_SINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                desc.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if(paramDesc.Mask <= 7)
        {
            if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                desc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                desc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if(paramDesc.Mask <= 15)
        {
            if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        layoutDescs.push_back(desc);
    }

    HRESULT hr = device->CreateInputLayout(
        layoutDescs.data(),
        static_cast<UINT>(layoutDescs.size()),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &pResources->inputLayout
    );

    if(FAILED(hr))
        throw Exception("Failed to create input layout");

    // create uniform info
    if(shaderDesc.ConstantBuffers > 0)
    {
        ID3D11ShaderReflectionConstantBuffer* cb = reflector->GetConstantBufferByIndex(0);

        D3D11_SHADER_BUFFER_DESC cbDesc;
        cb->GetDesc(&cbDesc);

        D3D11_BUFFER_DESC bufDesc = {};
        bufDesc.ByteWidth = cbDesc.Size;
        bufDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = device->CreateBuffer(&bufDesc, nullptr, &pResources->uniformBuffer);
        if(FAILED(hr))
            throw Exception("Failed to create constant buffer");

        size_t totalSize = 0;

        for(UINT i = 0; i < cbDesc.Variables; ++i)
        {
            ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(i);
            D3D11_SHADER_VARIABLE_DESC varDesc;
            var->GetDesc(&varDesc);

            totalSize += varDesc.Size;

            UniformInfo info;
            info.offset = (uint32_t)varDesc.StartOffset;
            info.size = (uint32_t)varDesc.Size;
            pResources->uniformInfo[varDesc.Name] = info;
        }

        pResources->uniformData.resize(totalSize);
    }

    std::unordered_map<int, std::string> textureNamesBySlot;
    std::unordered_map<int, std::string> samplerNamesBySlot;

    // vertex shader textures
    for(UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC desc;
        reflector->GetResourceBindingDesc(i, &desc);

        if(desc.Type == D3D_SIT_TEXTURE)
            textureNamesBySlot[desc.BindPoint] = desc.Name;
        else if(desc.Type == D3D_SIT_SAMPLER)
            samplerNamesBySlot[desc.BindPoint] = desc.Name;
    }

    D3DReflect(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflector);
    reflector->GetDesc(&shaderDesc);

    // pixel shader textures
    for(UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC desc;
        reflector->GetResourceBindingDesc(i, &desc);

        if(desc.Type == D3D_SIT_TEXTURE)
            textureNamesBySlot[desc.BindPoint] = desc.Name;
        else if(desc.Type == D3D_SIT_SAMPLER)
            samplerNamesBySlot[desc.BindPoint] = desc.Name;
    }

    // store texture slots with uniforms
    for(const auto& [slot, texName] : textureNamesBySlot)
    {
        auto it = samplerNamesBySlot.find(slot);
        if(it == samplerNamesBySlot.end()) {
            assert(false && "Sampler slot missing for texture slot");
            continue;
        }

        UniformInfo info;
        info.offset = (uint32_t)slot;
        info.size = 0;
        pResources->uniformInfo[texName] = info;
    }
}

void Graphics::UpdateBufferInternal(const ComPtr<ID3D11Buffer>& buffer, const std::vector<std::byte>& data)
{
    void *pData = MapBufferInternal(buffer, 0, BufferMapAccess::WriteDiscard);
    memcpy(pData, data.data(), data.size());
    UnmapBufferInternal(buffer, 0);
}

void Graphics::CreateSurfaceInternal(Window* window)
{
    HWND hWnd = window->hWnd;
    uint32_t width = (uint32_t)window->size.x;
    uint32_t height = (uint32_t)window->size.y;
    WindowResources* pResources = &window->resources;

    // create swap chain description
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;// DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    //swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    //swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    // create swap chain
    ComPtr<IDXGIDevice1> dxgiDevice;
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory2> dxgiFactory;
    device.As(&dxgiDevice);
    dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory);

    HRESULT res;

    res = dxgiFactory->CreateSwapChainForHwnd(
        device.Get(), hWnd,
        &swapChainDesc, nullptr, nullptr,
        &pResources->swapChain);

    if(FAILED(res))
        Throw("failed to create d3d11 swap chain");

    dxgiDevice->SetMaximumFrameLatency(1);

    // create render target view
    ComPtr<ID3D11Texture2D> backBufferTex;
    pResources->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTex);
    res = device->CreateRenderTargetView(backBufferTex.Get(), nullptr, &pResources->renderTargetView);

    if(FAILED(res))
        Throw("failed to create d3d11 render target view");

    // create depth stencil view
    CD3D11_TEXTURE2D_DESC depthTextureDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, width, height, 1, 1, D3D11_BIND_DEPTH_STENCIL);
    ComPtr<ID3D11Texture2D> depthStencilTex;
    res = device->CreateTexture2D(&depthTextureDesc, nullptr, &depthStencilTex);

    if(FAILED(res))
        Throw("failed to create texture for d3d11 depth stencil");

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    res = device->CreateDepthStencilView(
        depthStencilTex.Get(), &depthStencilViewDesc, &pResources->depthStencilView);

    if(FAILED(res))
        Throw("failed to create d3d11 depth stencil view");
}

void Graphics::DestroySurfaceInternal(Window* window)
{
    window->resources.swapChain = nullptr;
    window->resources.renderTargetView = nullptr;
    window->resources.depthStencilView = nullptr;
}

} // fraze

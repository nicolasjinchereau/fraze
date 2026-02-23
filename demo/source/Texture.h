/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Exception.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <Types.h>
#include <WindowsPlatform.h>
#include <array>
#include <string>

namespace fraze {

class Graphics;
class Shader;

class Texture : public Object
{
    friend Graphics;
    friend Shader;

    Graphics* graphics{};
    int width{};
    int height{};
    std::vector<std::byte> data;
    PixelDataFormat format = PixelDataFormat::RGBA32;
    TextureWrapMode wrapMode = TextureWrapMode::Clamp;
    TextureFilterMode filterMode = TextureFilterMode::Bilinear;
    float anisoLevel = 0;
    float maxAniso = 1;

    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> resourceView;
    ComPtr<ID3D11SamplerState> samplerState;
public:
    Texture(const TypeInfo* typeInfo, Graphics* graphics, std::string_view path);
    ~Texture();

    virtual WordType GetType() const override;

    static void CreateTextureAsync(Program* program, Class& task, Graphics* graphics, std::string_view path);
private:
    void UpdateSamplerState();
};

} // fraze

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
class Texture;

struct UniformInfo
{
    uint32_t offset;
    uint32_t size;
};

struct ShaderResources
{
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> uniformBuffer;
    std::vector<std::byte> uniformData;
    string_view_map<UniformInfo> uniformInfo;
};

class Shader : public Object
{
    friend Graphics;

    Graphics* graphics{};
    ShaderResources resources;
    bool uniformsChanged = true;
public:
    Shader(const TypeInfo* typeInfo, Graphics* graphics, std::string_view src, std::string_view vertexEntry, std::string_view pixelEntry);
    ~Shader();

    template<class T> requires is_any_of_v<T, Mat4, Color>
    void SetUniform(std::string_view name, const T& value) {
        auto data = AsFloatArray(value);
        SetUniform(name, data.data(), data.size() * sizeof(float));
    }

    void SetUniform(std::string_view name, Texture* texture);
    bool HasUniform(std::string_view name) const;

    static void CreateShaderAsync(Program* program, Class& task, Graphics* graphics, std::string_view src, std::string_view vertexEntry, std::string_view pixelEntry);
private:
    void SetUniform(std::string_view name, const void* data, size_t size);
};

} // fraze

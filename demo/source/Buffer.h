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

namespace fraze {

class Graphics;

class Buffer : public Object
{
    friend Graphics;

    Graphics* graphics{};
    BufferType type = {};
    BufferUsage usage = {};
    BufferCPUAccess cpuAccess = {};
    size_t size = {};
    size_t stride = {};
    ComPtr<ID3D11Buffer> buffer;
public:
    Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, Integer size);
    Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data);
    Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const std::vector<float>& data, size_t stride);
    Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const void* data, size_t size, size_t stride);
    ~Buffer();

    void SetData(const Array<>& data);
    size_t GetSize() const;
    size_t GetStride() const;
    size_t GetNativeSize() const;
    size_t GetNativeStride() const;
};

} // fraze

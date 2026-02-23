/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <Buffer.h>
#include <Graphics.h>

namespace fraze {

std::vector<float> ToFloatBuffer(const Array<>& data)
{
	size_t bytesPerWord = 8;
	size_t floatsPerNum = 2;
	size_t size = data.GetSize() * bytesPerWord / floatsPerNum;

	const Number* first = data.At(0).GetData<const Number>();
	const Number* last = first + data.GetSize();

	std::vector<float> init(last - first);
	std::transform(first, last, init.begin(), [](Number num) { return static_cast<float>(num); });
	return init;
}

Buffer::Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, Integer size)
	: Buffer(typeInfo, graphics, type, usage, cpuAccess, std::vector<float>((size_t)(size * 8 / 2)), 0)
{
}

Buffer::Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const Array<>& data)
	: Buffer(typeInfo, graphics, type, usage, cpuAccess, ToFloatBuffer(data), data.GetSize() / data.GetCount() * 8 / 2)
{
}

Buffer::Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const std::vector<float>& data, size_t stride)
	: Buffer(typeInfo, graphics, type, usage, cpuAccess, data.data(), data.size() * sizeof(float), stride)
{
}

Buffer::Buffer(const TypeInfo* typeInfo, Graphics* graphics, BufferType type, BufferUsage usage, BufferCPUAccess cpuAccess, const void* data, size_t size, size_t stride)
	: Object(typeInfo)
    , graphics(graphics)
	, type(type)
	, usage(usage)
	, cpuAccess(cpuAccess)
	, size(size)
	, stride(stride)
{
	buffer = graphics->CreateBuffer(type, usage, cpuAccess, data, size);
}

Buffer::~Buffer()
{
}

void Buffer::SetData(const Array<>& data)
{
	size_t strideInWords = data.GetSize() / data.GetCount();
	size_t bytesPerWord = 8;
	size_t floatsPerNum = 2;
	stride = strideInWords * bytesPerWord / floatsPerNum;
	
	const Number* vertComponents = data.At(0).GetData<Number>();
	size_t requiredDataSize = data.GetSize() * 8 / 2;
	ENFORCE(requiredDataSize <= size, SourceLocation(), "Data size '{}' is larger than buffer size '{}'", requiredDataSize, size);

	void *pData = graphics->MapBuffer(buffer, 0, BufferMapAccess::WriteDiscard);
	
	float* dest = static_cast<float*>(pData);
	ToFloatBuffer(vertComponents, data.GetSize(), dest, size);

	graphics->UnmapBuffer(buffer, 0);
}

WordType Buffer::GetType() const {
	return WordType::Object;
}

size_t Buffer::GetSize() const {
	return size * 2 / 8;
}

size_t Buffer::GetStride() const {
	return stride * 2 / 8;
}

size_t Buffer::GetNativeSize() const {
	return size;
}

size_t Buffer::GetNativeStride() const {
	return stride;
}

} // fraze

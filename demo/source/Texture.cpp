/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <Texture.h>
#include <Graphics.h>
#include <fraze/common/Pointers.h>
#include <fraze/program/Dispatcher.h>
#include <fraze/program/Program.h>
#include <WorkerThread.h>
#include <fstream>
#include <future>
#include <png.h>

namespace fraze {

void ExpectBytes(const std::span<std::byte>& byte, ptrdiff_t count)
{
    if (byte.size() < (size_t)count)
        throw Exception("unexpected end of data");
}

size_t GetBytesPerPixel(PixelDataFormat format)
{
    switch(format)
    {
    case PixelDataFormat::Alpha8:    return 1;
    case PixelDataFormat::RGB24:     return 3;
    case PixelDataFormat::RGBA32:    return 4;
    case PixelDataFormat::RGBAFloat: return 16;
    default:                         return 0;
    }
}

Texture::Texture(Graphics* graphics, std::string_view path)
{
    this->graphics = graphics;

    // LOAD FILE DATA
    std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
    if(!file)
        throw Exception("Failed to open file");

    std::streamsize size = file.tellg();
    if(size < 0)
        throw Exception("Failed to get file size");

    std::vector<std::byte> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    if(!file)
        throw Exception("Failed to read file");

    std::span<std::byte> fileData = buffer;

    // DECODE PNG FROM FILE DATA
    png_struct* pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!pPngStruct)
        throw Exception("failed to initialize libpng.");

    png_set_option(pPngStruct, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);

    png_info* pPngInfo = png_create_info_struct(pPngStruct);
    if(!pPngInfo) {
        png_destroy_read_struct(&pPngStruct, NULL, NULL);
        throw Exception("failed to initialize libpng.");
    }

    const int PngSigSize = 8;

    ExpectBytes(fileData, PngSigSize);

    if(png_sig_cmp((png_bytep)fileData.data(), 0, PngSigSize) != 0)
        throw Exception("error: invalid png file.");

    uptr<std::byte*[]> rowPtrs;
    std::vector<std::byte> tmpData;
    uint32_t tmpWidth{};
    uint32_t tmpHeight{};

    if(setjmp(png_jmpbuf(pPngStruct)) == 0)
    {
        png_set_read_fn(pPngStruct, &fileData, [](png_structp png_ptr, png_bytep out_bytes, png_size_t length)
            {
                std::span<std::byte>& fileData = *(std::span<std::byte>*)png_get_io_ptr(png_ptr);

                if (fileData.size() < length) {
                    png_error(png_ptr, "unexpected end of data");
                    return;
                }

                memcpy(out_bytes, fileData.data(), length);
                fileData = fileData.subspan(length);
            });

        png_set_sig_bytes(pPngStruct, PngSigSize);
        fileData = fileData.subspan(PngSigSize);

        // get image dimensions
        png_read_info(pPngStruct, pPngInfo);
        tmpWidth = png_get_image_width(pPngStruct, pPngInfo);
        tmpHeight = png_get_image_height(pPngStruct, pPngInfo);

        // bits per channel
        uint32_t bitDepth = png_get_bit_depth(pPngStruct, pPngInfo);

        // (RGB, RGBA, Luminance, luminance alpha... palette... etc)
        uint32_t colorType = png_get_color_type(pPngStruct, pPngInfo);

        // convert to 8 bits per channel
        if(bitDepth == 16)
            png_set_strip_16(pPngStruct);

        // expand grayscale
        if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
            png_set_expand_gray_1_2_4_to_8(pPngStruct);

        // expand to RGBA
        if(png_get_valid(pPngStruct, pPngInfo, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(pPngStruct);

        // fill last byte of 24bit images
        png_set_filler(pPngStruct, 0xFF, PNG_FILLER_AFTER);

        // required for palette alterations
        png_read_update_info(pPngStruct, pPngInfo);

        // allocate storage for pixels
        tmpData.resize(tmpWidth * tmpHeight * 4);

        // make pointers to pixel rows
        rowPtrs = std::make_unique<std::byte*[]>(tmpHeight);

        for(uint32_t y = 0; y < tmpHeight; ++y)
            rowPtrs[y] = (std::byte*)(tmpData.data() + y * tmpWidth * 4);

        png_read_image(pPngStruct, (png_bytepp)rowPtrs.get());

        png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    }
    else
    {
        std::string error = (const char*)png_get_error_ptr(pPngStruct);
        png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
        throw Exception(!error.empty() ? error : "could not read png file");
    }

    width = (int)tmpWidth;
    height = (int)tmpHeight;
    format = PixelDataFormat::RGBA32;
    data = std::move(tmpData);

    graphics->CreateTexture(width, height, format, data, texture.GetAddressOf(), resourceView.GetAddressOf());

    UpdateSamplerState();
}

Texture::~Texture()
{

}

WordType Texture::GetType() const {
    return WordType::Object;
}

void Texture::CreateTextureAsync(Program* program, Class& task, Graphics* graphics, std::string_view path)
{
    Class* taskPtr = &task;
    program->PinMemory(taskPtr);

    std::string pathStr { path };

    sptr<Dispatcher> dispatcher = Dispatcher::GetCurrent();
    
    WorkerThread::GetInstance().InvokeAsync([=]{
        Texture* texture = new Texture(graphics, pathStr);

        dispatcher->InvokeAsync([=] {
            taskPtr->SetField("$position", Integer(-1));
            taskPtr->SetField("$value", texture);
            program->Invoke("OnAwaitableCompleted", taskPtr);
            program->UnpinMemory(taskPtr);
        });
    });
}

void Texture::UpdateSamplerState()
{
    graphics->CreateSamplerState(filterMode, wrapMode, samplerState.GetAddressOf());
}

} // fraze

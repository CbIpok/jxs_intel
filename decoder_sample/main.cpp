#include "SvtJpegxsDec.h"
#include <stdio.h>
#include <corecrt_malloc.h>
#include <vector>
#include <intsafe.h>
#include <wincodec.h>

// Утилита для ограничения значений от 0 до 255
uint8_t clamp(double arg)
{
    if (arg < 0.0)
        return 0;
    else if (arg > 255.0)
        return 255;
    return (uint8_t)arg;
}

// Функция загрузки битстрима из файла
bool loadBitstream(const char* input_file_name, svt_jpeg_xs_bitstream_buffer_t& bitstream)
{
    FILE* input_file = nullptr;
#ifdef _WIN32
    fopen_s(&input_file, input_file_name, "rb");
#else
    input_file = fopen(input_file_name, "rb");
#endif
    if (!input_file) {
        printf("Can not open input file: %s!\n", input_file_name);
        return false;
    }

    bitstream.allocation_size = 1000;
    bitstream.buffer = (uint8_t*)malloc(bitstream.allocation_size);
    if (!bitstream.buffer) {
        fclose(input_file);
        return false;
    }

    size_t read_size = fread(bitstream.buffer, 1, bitstream.allocation_size, input_file);
    printf("Read file %s size: %lu!\n", input_file_name, (unsigned long)read_size);
    fseek(input_file, 0, SEEK_SET);

    uint32_t frame_size = 0;
    SvtJxsErrorType_t err = svt_jpeg_xs_decoder_get_single_frame_size(
        bitstream.buffer, static_cast<uint32_t>(read_size), nullptr, &frame_size, 1);
    if (err || frame_size == 0) {
        free(bitstream.buffer);
        fclose(input_file);
        return false;
    }

    bitstream.allocation_size = frame_size;
    bitstream.buffer = (uint8_t*)realloc(bitstream.buffer, bitstream.allocation_size);
    if (!bitstream.buffer) {
        fclose(input_file);
        return false;
    }

    read_size = fread(bitstream.buffer, 1, bitstream.allocation_size, input_file);
    if (read_size != bitstream.allocation_size) {
        free(bitstream.buffer);
        fclose(input_file);
        return false;
    }
    bitstream.used_size = read_size;
    fclose(input_file);
    return true;
}

// Функция декодирования изображения
bool decodeImage(svt_jpeg_xs_bitstream_buffer_t& bitstream, svt_jpeg_xs_decoder_api_t& dec, svt_jpeg_xs_image_config_t& image_config, svt_jpeg_xs_image_buffer_t& image_buffer)
{
    SvtJxsErrorType_t err = svt_jpeg_xs_decoder_init(
        SVT_JPEGXS_API_VER_MAJOR, SVT_JPEGXS_API_VER_MINOR, &dec, bitstream.buffer, bitstream.used_size, &image_config);
    if (err) {
        return false;
    }

    uint32_t pixel_size = image_config.bit_depth <= 8 ? 1 : 2;
    for (uint8_t i = 0; i < image_config.components_num; ++i) {
        image_buffer.stride[i] = image_config.components[i].width;
        image_buffer.alloc_size[i] = image_buffer.stride[i] * image_config.components[i].height * pixel_size;
        image_buffer.data_yuv[i] = malloc(image_buffer.alloc_size[i]);
        if (!image_buffer.data_yuv[i]) {
            return false;
        }
    }

    svt_jpeg_xs_frame_t dec_input;
    dec_input.bitstream = bitstream;
    dec_input.image = image_buffer;
    err = svt_jpeg_xs_decoder_send_frame(&dec, &dec_input, 1);
    if (err) {
        return false;
    }

    svt_jpeg_xs_frame_t dec_output;
    err = svt_jpeg_xs_decoder_get_frame(&dec, &dec_output, 1);
    return (err == SvtJxsErrorNone);
}

// Функция сохранения изображения в формате PNG
bool saveImageAsPNG(const svt_jpeg_xs_image_config_t& image_config, const svt_jpeg_xs_image_buffer_t& image_buffer, const char* output_filename)
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        return false;
    }

    IWICImagingFactory* wicFactory = nullptr;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) {
        return false;
    }

    std::vector<uint8_t> pxldata(4 * image_config.width * image_config.height);
    size_t total_pixels = image_config.width * image_config.height;
    uint32_t pixel_size = image_config.bit_depth <= 8 ? 1 : 2;
    int rightshift = (image_config.bit_depth > 8) ? (image_config.bit_depth - 8) : 0;

    if (pixel_size == 1) {
        memcpy(pxldata.data(), image_buffer.data_yuv[0], total_pixels);
        memcpy(pxldata.data() + total_pixels, image_buffer.data_yuv[1], total_pixels);
        memcpy(pxldata.data() + 2 * total_pixels, image_buffer.data_yuv[2], total_pixels);

        if (image_config.components_num == 4) {
            memcpy(pxldata.data() + 3 * total_pixels, image_buffer.data_yuv[3], total_pixels);
        }
        else {
            for (size_t ix = 0; ix < total_pixels; ++ix) {
                pxldata[4 * ix + 3] = 255;
            }
        }
    }
    else {
        uint16_t* y_plane = static_cast<uint16_t*>(image_buffer.data_yuv[0]);
        uint16_t* u_plane = static_cast<uint16_t*>(image_buffer.data_yuv[1]);
        uint16_t* v_plane = static_cast<uint16_t*>(image_buffer.data_yuv[2]);
        uint16_t* a_plane = (image_config.components_num == 4) ? static_cast<uint16_t*>(image_buffer.data_yuv[3]) : nullptr;

        for (size_t ix = 0; ix < total_pixels; ++ix) {
            pxldata[4 * ix + 0] = y_plane[ix] >> rightshift;
            pxldata[4 * ix + 1] = u_plane[ix] >> rightshift;
            pxldata[4 * ix + 2] = v_plane[ix] >> rightshift;
            pxldata[4 * ix + 3] = (a_plane) ? (a_plane[ix] >> rightshift) : 255;
        }
    }

    IWICBitmap* pngBitmap = nullptr;
    hr = wicFactory->CreateBitmapFromMemory(
        image_config.width, image_config.height, GUID_WICPixelFormat32bppPRGBA,
        4 * image_config.width, static_cast<UINT>(pxldata.size()), pxldata.data(), &pngBitmap);
    if (FAILED(hr)) {
        return false;
    }

    IWICStream* wicStream = nullptr;
    hr = wicFactory->CreateStream(&wicStream);
    if (FAILED(hr)) {
        return false;
    }

    hr = wicStream->InitializeFromFilename(L"decodedImage.png", GENERIC_WRITE);
    if (FAILED(hr)) {
        return false;
    }

    IWICBitmapEncoder* encoder = nullptr;
    hr = wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    if (FAILED(hr)) {
        return false;
    }

    hr = encoder->Initialize(wicStream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) {
        return false;
    }

    IWICBitmapFrameEncode* frame = nullptr;
    hr = encoder->CreateNewFrame(&frame, nullptr);
    if (FAILED(hr)) {
        return false;
    }

    hr = frame->Initialize(nullptr);
    if (FAILED(hr)) {
        return false;
    }

    hr = frame->WriteSource(pngBitmap, nullptr);
    if (FAILED(hr)) {
        return false;
    }

    hr = frame->Commit();
    hr = encoder->Commit();

    wicStream->Release();
    frame->Release();
    encoder->Release();
    pngBitmap->Release();
    wicFactory->Release();
    CoUninitialize();

    return true;
}

// Основная функция
int main(int32_t argc, char* argv[])
{
      if (argc < 2) {
      printf("Not set input file!\n");
      return -1;
    }

    svt_jpeg_xs_bitstream_buffer_t bitstream;
    if (!loadBitstream(argv[1], bitstream)) {
        printf("Failed to load bitstream\n");
        return -1;
    }

    svt_jpeg_xs_decoder_api_t dec = { 0 };
    dec.verbose = VERBOSE_SYSTEM_INFO;
    dec.threads_num = 10;
    dec.use_cpu_flags = CPU_FLAGS_ALL;

    svt_jpeg_xs_image_config_t image_config;
    svt_jpeg_xs_image_buffer_t image_buffer;

    if (!decodeImage(bitstream, dec, image_config, image_buffer)) {
        printf("Failed to decode image\n");
        free(bitstream.buffer);
        for (uint8_t i = 0; i < image_config.components_num; ++i) {
            free(image_buffer.data_yuv[i]);
        }
        return -1;
    }

    if (!saveImageAsPNG(image_config, image_buffer, "decodedImage.png")) {
        printf("Failed to save image as PNG\n");
        free(bitstream.buffer);
        for (uint8_t i = 0; i < image_config.components_num; ++i) {
            free(image_buffer.data_yuv[i]);
        }
        return -1;
    }

    free(bitstream.buffer);
    for (uint8_t i = 0; i < image_config.components_num; ++i) {
        free(image_buffer.data_yuv[i]);
    }
    svt_jpeg_xs_decoder_close(&dec);

    return 0;
}

#include <vector>
#include <cstdint>
#include <cmath>
#include "SvtJpegxsDec.h"

// Утилита для ограничения значений от 0 до 255
inline uint8_t clamp(float value)
{
    return static_cast<uint8_t>(std::fmin(255.0f, std::fmax(0.0f, value)));
}


void PlanarToPackaged(svt_jpeg_xs_image_config_t& image_config, svt_jpeg_xs_image_buffer_t& image_buffer, std::vector<uint8_t>& pxldata)
{
    // Убедимся, что размер вектора данных пикселей подходит для хранения изображения в формате RGBA
    pxldata.resize(image_config.width * image_config.height * 4);

    // Простое преобразование YUV в упакованный формат RGB
    for (uint32_t y = 0; y < image_config.height; ++y)
    {
        for (uint32_t x = 0; x < image_config.width; ++x)
        {
            uint32_t index = y * image_config.width + x;

            // Предполагаем, что компоненты Y, U и V расположены в отдельных плоскостях
            uint8_t y_value = static_cast<uint8_t*>(image_buffer.data_yuv[0])[index];
            uint8_t u_value = static_cast<uint8_t*>(image_buffer.data_yuv[1])[index / 2];
            uint8_t v_value = static_cast<uint8_t*>(image_buffer.data_yuv[2])[index / 2];

            // Простейшее преобразование YUV в RGB с использованием формул
            pxldata[4 * index + 0] = clamp(y_value + 1.402f * (v_value - 128)); // R
            pxldata[4 * index + 1] = clamp(y_value - 0.344136f * (u_value - 128) - 0.714136f * (v_value - 128)); // G
            pxldata[4 * index + 2] = clamp(y_value + 1.772f * (u_value - 128)); // B
            pxldata[4 * index + 3] = 255; // A (полная непрозрачность)
        }
    }
}
void PlanarToPackaged_(svt_jpeg_xs_image_config_t& image_config, svt_jpeg_xs_image_buffer_t& image_buffer, std::vector<uint8_t>& pxldata)
{
    // Определение сдвига вправо на основе глубины битов
    int rightshift = (image_config.bit_depth > 8) ? (image_config.bit_depth - 8) : 0;

    // Проверка формата цветности и обработка пикселей
    if (image_config.format == COLOUR_FORMAT_PLANAR_YUV444_OR_RGB)
    {
        for (int ix = 0; ix < image_config.width * image_config.height; ++ix)
        {
            pxldata[4 * ix] = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(image_buffer.data_yuv[0])[ix] >> rightshift);
            pxldata[4 * ix + 1] = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(image_buffer.data_yuv[1])[ix] >> rightshift);
            pxldata[4 * ix + 2] = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(image_buffer.data_yuv[2])[ix] >> rightshift);
            pxldata[4 * ix + 3] = (image_config.components_num == 4)
                ? static_cast<uint32_t>(reinterpret_cast<uint8_t*>(image_buffer.data_yuv[3])[ix] >> rightshift)
                : 255;
        }
    }
    else if (image_config.format == COLOUR_FORMAT_PLANAR_YUV420)
    {
        for (int ix = 0; ix < image_config.width * image_config.height; ++ix)
        {
            uint32_t y = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[0])[ix] >> rightshift);
            uint32_t u = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[1])[ix / 2] >> rightshift);
            uint32_t v = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[2])[ix / 2] >> rightshift);
            pxldata[4 * ix] = clamp(y + 1.28033f * (v - 128));
            pxldata[4 * ix + 1] = clamp(y - 0.21482f * (u - 128) - 0.38059f * (v - 128));
            pxldata[4 * ix + 2] = clamp(y + 2.12798f * (u - 128));
            pxldata[4 * ix + 3] = (image_config.components_num == 4)
                ? static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[3])[ix] >> rightshift)
                : 255;
        }
    }
    else if (image_config.format == COLOUR_FORMAT_PLANAR_YUV422)
    {
        for (int ix = 0; ix < image_config.width * image_config.height; ++ix)
        {
            uint32_t y = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[0])[ix] >> rightshift);
            uint32_t u = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[1])[ix / 2] >> rightshift);
            uint32_t v = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[2])[ix / 2] >> rightshift);
            pxldata[4 * ix] = clamp(y + 1.28033f * (v - 128));
            pxldata[4 * ix + 1] = clamp(y - 0.21482f * (u - 128) - 0.38059f * (v - 128));
            pxldata[4 * ix + 2] = clamp(y + 2.12798f * (u - 128));
            pxldata[4 * ix + 3] = (image_config.components_num == 4)
                ? static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[3])[ix] >> rightshift)
                : 255;
        }
    }
    else if (image_config.format == COLOUR_FORMAT_PLANAR_YUV420)
    {
        bool oddrow = false;
        int y_offset = 0;
        int uv_offset = 0;
        int y_index = 0;
        int uv_index = 0;

        for (; y_offset < image_config.height; ++y_offset, y_index += image_config.width)
        {
            for (int ix = 0; ix < image_config.width; ++ix)
            {
                int y_pos = y_index + ix;
                int uv_pos = uv_index + ix / 2;
                uint32_t y = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[0])[y_pos] >> rightshift);
                uint32_t u = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[1])[uv_pos] >> rightshift);
                uint32_t v = static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[2])[uv_pos] >> rightshift);
                pxldata[4 * y_pos] = clamp(y + 1.28033f * (v - 128));
                pxldata[4 * y_pos + 1] = clamp(y - 0.21482f * (u - 128) - 0.38059f * (v - 128));
                pxldata[4 * y_pos + 2] = clamp(y + 2.12798f * (u - 128));
                pxldata[4 * y_pos + 3] = (image_config.components_num == 4)
                    ? static_cast<uint32_t>(reinterpret_cast<uint32_t*>(image_buffer.data_yuv[3])[ix] >> rightshift)
                    : 255;
            }
            if (oddrow)
            {
                ++uv_offset;
                uv_index += image_config.width / 2;
            }
            oddrow = !oddrow;
        }
    }
}

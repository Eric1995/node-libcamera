
#include <libcamera/formats.h>

#include "../core/still_options.hpp"
#include "../core/stream_info.hpp"
#include "qoi.h"

// YUV420P 转换为 RGB
// void YUV420PToRGB(unsigned char *yuv420p, unsigned char *rgb, int width, int height)
// {
//     int i, j;
//     int y, u, v;
//     int r, g, b;

//     for (i = 0; i < height; i++)
//     {
//         for (j = 0; j < width; j++)
//         {
//             y = yuv420p[i * width + j];
//             u = yuv420p[height * width + (i / 2) * (width / 2) + j / 2];
//             v = yuv420p[height * width + height * width / 4 + (i / 2) * (width / 2) + j / 2];

//             r = y + 1.402 * (v - 128);
//             g = y - 0.34414 * (u - 128) - 0.71414 * (v - 128);
//             b = y + 1.772 * (u - 128);

//             r = (r < 0) ? 0 : ((r > 255) ? 255 : r);
//             g = (g < 0) ? 0 : ((g > 255) ? 255 : g);
//             b = (b < 0) ? 0 : ((b > 255) ? 255 : b);

//             rgb[(i * width + j) * 3] = r;
//             rgb[(i * width + j) * 3 + 1] = g;
//             rgb[(i * width + j) * 3 + 2] = b;
//         }
//     }
// }

bool encodeQOI(unsigned char *rgb, int width, int height, const char *filename)
{
    qoi_desc desc;
    desc.width = width;
    desc.height = height;
    desc.channels = 3; // RGB
    desc.colorspace = QOI_SRGB;

    int qoi_size;
    void *qoi_data = qoi_encode(rgb, &desc, &qoi_size);

    if (qoi_data == nullptr)
    {
        return false;
    }

    FILE *f = fopen(filename, "wb");
    if (f == nullptr)
    {
        free(qoi_data);
        return false;
    }

    fwrite(qoi_data, 1, qoi_size, f);
    fclose(f);
    free(qoi_data);

    return true;
}

void qoi_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info, std::string const &filename, StillOptions const *options)
{
    unsigned w = info.width, h = info.height, stride = info.stride;
    // unsigned char *rgb = new unsigned char[w * h * 3];
    // YUV420PToRGB((uint8_t *)(mem[0].data()), rgb, w, h);
    encodeQOI((uint8_t *)(mem[0].data()), w, h, filename.c_str());
    // delete[] rgb;
}
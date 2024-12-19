#include <libcamera/control_ids.h>
#include <libcamera/formats.h>
#include <stdio.h>
#include <stdlib.h>
#include <turbojpeg.h>

#include "../core/still_options.hpp"
#include "../core/stream_info.hpp"

using namespace libcamera;

static void compress_jpeg_turbo_rgb888(unsigned char *image_buffer, int width, int height, int quality, const char *filename)
{
    tjhandle tjInstance = tjInitCompress();
    unsigned char *jpegBuf = NULL;
    unsigned long jpegSize = 0;

    if (tjCompress2(tjInstance, image_buffer, width, 0, height, TJPF_RGB, &jpegBuf, &jpegSize, TJSAMP_420, quality, 0) < 0)
    {
        printf("libjpeg-turbo compress failed: %s\n", tjGetErrorStr());
        tjDestroy(tjInstance);
        return;
    }
    FILE *fp = fopen(filename, "wb");
    if (fp)
    {
        fwrite(jpegBuf, 1, jpegSize, fp);
        fclose(fp);
    }
    else
    {
        fprintf(stderr, "could not save %s !\n", filename);
        tjFree(jpegBuf);
        tjDestroy(tjInstance);
        return;
    }
    tjFree(jpegBuf);
    tjDestroy(tjInstance);
}

void jpeg_turbo_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info, libcamera::ControlList const &metadata, std::string const &filename, std::string const &cam_model,
                     StillOptions const *options)
{
    unsigned w = info.width, h = info.height, stride = info.stride;
    compress_jpeg_turbo_rgb888((uint8_t *)(mem[0].data()), w, h, options->quality, filename.c_str());
}

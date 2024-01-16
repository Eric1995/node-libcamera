#ifndef _H264_ENCODER_H_
#define _H264_ENCODER_H_ 1
#include "util.hpp"
#include <fcntl.h>
#include <libcamera/libcamera.h>
#include <linux/videodev2.h>
#include <napi.h>
#include <optional>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace Napi;

struct frame_data_t
{
    uint32_t size;
    uint8_t *data;
};

using FrameType = frame_data_t *;

struct buffer
{
    void *start;
    int length;
    struct v4l2_buffer inner;
    struct v4l2_plane plane;
};

static int get_v4l2_colorspace(std::optional<libcamera::ColorSpace> const &cs)
{
    if (cs == libcamera::ColorSpace::Rec709)
        return V4L2_COLORSPACE_REC709;
    else if (cs == libcamera::ColorSpace::Smpte170m)
        return V4L2_COLORSPACE_SMPTE170M;

    // LOG(1, "H264: surprising colour space: " << libcamera::ColorSpace::toString(cs));
    return V4L2_COLORSPACE_SMPTE170M;
}

// mmaps the buffers for the given type of device (capture or output).
void map(int fd, uint32_t type, struct buffer *buffer, bool dma)
{
    struct v4l2_buffer *inner = &buffer->inner;

    memset(inner, 0, sizeof(*inner));
    inner->type = type;
    if (dma)
    {
        inner->memory = V4L2_MEMORY_DMABUF;
    }
    else
    {
        inner->memory = V4L2_MEMORY_MMAP;
    }

    inner->index = 0;
    inner->length = 1;
    inner->m.planes = &buffer->plane;
    ioctl(fd, VIDIOC_QUERYBUF, inner);
    std::cout << "map plane length after query buffer: " << ((&(buffer->plane))[0]).length << std::endl;
    buffer->length = inner->m.planes[0].length;
    buffer->start = mmap(NULL, buffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, inner->m.planes[0].m.mem_offset);
    if (buffer->start == (void *)-1)
    {
        printf("mmap error : %d-%s. \r\n", errno, strerror(errno));
    }
}

struct buffer output;
struct buffer capture;

class EncoderWorker : public AsyncProgressWorker<FrameType>
{
  public:
    uint32_t width = 640;
    uint32_t height = 480;
    uint32_t bitrate_bps = 4 * 1024 * 1024;
    uint32_t pixel_format = V4L2_PIX_FMT_YUYV;
    uint8_t num_planes = 1;
    int fd = open("/dev/video11", O_RDWR);
    bool stopped = false;
    // FILE *yuvFileFd = fopen("./test.h264", "w");
    EncoderWorker(Napi::Object option, Napi::Function callback) : AsyncProgressWorker(callback)
    {
        if (option.Get("width").IsNumber())
            width = option.Get("width").As<Napi::Number>().Uint32Value();
        if (option.Get("height").IsNumber())
            height = option.Get("height").As<Napi::Number>().Uint32Value();
        if (option.Get("bitrate").IsNumber())
            bitrate_bps = option.Get("bitrate").As<Napi::Number>().Uint32Value();
        if (option.Get("pixel_format").IsNumber())
            pixel_format = option.Get("pixel_format").As<Napi::Number>().Uint32Value();
        if (option.Get("num_planes").IsNumber())
            num_planes = option.Get("num_planes").As<Napi::Number>().Uint32Value();

        if (option.Get("controllers").IsArray())
        {
            Napi::Array controllers = option.Get("controllers").As<Napi::Array>();
            for (int i = 0; i < controllers.Length(); i++)
            {
                Napi::Object ctrl_obj = controllers.Get(i).As<Napi::Object>();
                uint32_t id = ctrl_obj.Get('id').As<Napi::Number>().Uint32Value();
                int32_t value = ctrl_obj.Get('value').As<Napi::Number>().Int32Value();
                v4l2_control ctrl = {};
                ctrl.id = id;
                ctrl.value = value;
                ioctl(fd, VIDIOC_S_CTRL, &ctrl);
            }
        }
        v4l2_control ctrl = {};
        ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
        ctrl.value = bitrate_bps;
        ioctl(fd, VIDIOC_S_CTRL, &ctrl);

        struct v4l2_format fm;
        struct v4l2_pix_format_mplane *mp = &fm.fmt.pix_mp;

        fm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        ioctl(fd, VIDIOC_G_FMT, &fm);

        mp->width = width;
        mp->height = height;
        mp->pixelformat = pixel_format;
        mp->field = V4L2_FIELD_ANY;
        mp->num_planes = num_planes;
        if (option.Get("plane_config").IsArray())
        {
            auto plane_config = option.Get("plane_config").As<Napi::Array>();
            for (int i = 0; i < plane_config.Length(); i++)
            {
                auto plane_c = plane_config.Get(i).As<Napi::Object>();
                if (plane_c.Get("bytesperline").IsNumber())
                {
                    mp->plane_fmt[i].bytesperline = plane_c.Get("bytesperline").As<Napi::Number>().Uint32Value();
                }
                if (plane_c.Get("sizeimage").IsNumber())
                {
                    mp->plane_fmt[i].sizeimage = plane_c.Get("sizeimage").As<Napi::Number>().Uint32Value();
                }
            }
        }
        if (option.Get("colorspace").IsNumber())
        {
            mp->colorspace = option.Get("colorspace").As<Napi::Number>().Uint32Value();
        }
        // mp->plane_fmt[0].bytesperline = config->stride;

        ioctl(fd, VIDIOC_S_FMT, &fm);

        fm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(fd, VIDIOC_G_FMT, &fm);

        mp->width = width;
        mp->height = height;
        mp->pixelformat = V4L2_PIX_FMT_H264;
        mp->field = V4L2_FIELD_ANY;
        mp->colorspace = V4L2_COLORSPACE_DEFAULT;
        mp->num_planes = 1;
        mp->plane_fmt[0].bytesperline = 0;
        mp->plane_fmt[0].sizeimage = 512 << 10;
        ioctl(fd, VIDIOC_S_FMT, &fm);

        struct v4l2_streamparm params;
        memset(&params, 0, sizeof(params));
        params.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        params.parm.output.timeperframe.numerator = 1;
        params.parm.output.timeperframe.denominator = 30;

        ioctl(fd, VIDIOC_S_PARM, &params);

        struct v4l2_requestbuffers buf;

        buf.count = 1;

        // struct buffer output;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_REQBUFS, &buf);
        map(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &output, false);

        // struct buffer capture;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_REQBUFS, &buf);
        map(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &capture, false);

        ioctl(fd, VIDIOC_QBUF, capture.inner);
        ioctl(fd, VIDIOC_QBUF, output.inner);

        int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        ioctl(fd, VIDIOC_STREAMON, &type);

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(fd, VIDIOC_STREAMON, &type);
    }

    void Execute(const ExecutionProgress &progress)
    {
        while (true)
        {
            if (stopped)
            {
                // fclose(yuvFileFd);
                break;
            }
            usleep(5 * 1000);
            struct v4l2_buffer buf;
            buf.memory = V4L2_MEMORY_DMABUF;
            buf.length = 1;

            struct v4l2_plane out_planes;
            memset(&out_planes, 0, sizeof(out_planes));
            buf.m.planes = &out_planes;

            // Dequeue the output buffer, read the next frame and queue it back.
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            ioctl(fd, VIDIOC_DQBUF, &buf);
            // struct v4l2_buffer buf;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.length = 1;
            // struct v4l2_plane out_planes;
            memset(&out_planes, 0, sizeof(out_planes));
            buf.m.planes = &out_planes;
            // Dequeue the capture buffer, write out the encoded frame and queue it back.
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            ioctl(fd, VIDIOC_DQBUF, &buf);

            int encoded_len = buf.m.planes[0].bytesused;
            if (encoded_len > 0)
            {
                // fwrite((&capture)->start, sizeof(uint8_t), encoded_len, yuvFileFd);
                uint8_t *frame_buffer = new uint8_t[encoded_len];
                memcpy(frame_buffer, (&capture)->start, encoded_len);
                FrameType frame_data = new frame_data_t{encoded_len, frame_buffer};
                progress.Send(&frame_data, sizeof(frame_data_t));
            }
            ioctl(fd, VIDIOC_QBUF, (&capture)->inner);
        }
    }

    void feed(Napi::Object data)
    {
        Napi::Array planes = data.Get("planes").As<Napi::Array>();
        uint32_t index = 0;
        for (int i = 0; i < planes.Length(); i++)
        {
            Napi::Object plane = planes.Get(i).As<Napi::Object>();
            uint32_t size = plane.Get("size").As<Napi::Number>().Uint32Value();
            uint32_t plane_index = plane.Get("plane_index").As<Napi::Number>().Uint32Value();
            uint8_t *plane_data = (uint8_t *)plane.Get("data").As<Napi::ArrayBuffer>().Data();
            neon_memcpy(output.start + index, plane_data, size);
            // output.inner.m.planes[i].m.mem_offset = index;
            index += size;
        }
        ioctl(fd, VIDIOC_QBUF, (&output)->inner);
    }

    int setController(Napi::Object ctrl_obj)
    {
        auto id = ctrl_obj.Get("id").As<Napi::Number>().Uint32Value();
        auto value = ctrl_obj.Get("value").As<Napi::Number>().Int32Value();
        auto code = ctrl_obj.Get("code").As<Napi::Number>().Uint32Value();
        v4l2_control ctrl = {};
        ctrl.id = id;
        ctrl.value = value;
        return ioctl(fd, code, &ctrl);
    }

    void stop()
    {
        stopped = true;
        usleep(100 * 1000);
        int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        struct v4l2_requestbuffers buf;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        ioctl(fd, VIDIOC_REQBUFS, &buf);
        buf.memory = V4L2_MEMORY_MMAP;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(fd, VIDIOC_REQBUFS, &buf);
    }
    void OnError(const Error &e)
    {
        HandleScope scope(Env());
        Callback().Call({String::New(Env(), e.Message())});
    }
    void OnOK()
    {
        HandleScope scope(Env());
        Callback().Call({Env().Null(), String::New(Env(), "Ok")});
    }
    void OnProgress(const FrameType *data, size_t t)
    {
        HandleScope scope(Env());
        Napi::ArrayBuffer buffer =
            Napi::ArrayBuffer::New(Env(), (*data)->data, (*data)->size, [](Napi::Env env, void *arg) { delete[] arg; });
        Callback().Call({Env().Null(), Env().Null(), buffer});
        delete (*data);
    }
};

class H264Encoder : public Napi::ObjectWrap<H264Encoder>
{
  public:
    static Napi::FunctionReference *constructor;
    EncoderWorker *worker;
    H264Encoder(const Napi::CallbackInfo &info) : Napi::ObjectWrap<H264Encoder>(info)
    {
        Napi::Object option = info[0].As<Napi::Object>();
        Napi::Function callback = info[1].As<Napi::Function>();
        Napi::HandleScope scope(info.Env());
        worker = new EncoderWorker(option, callback);
        worker->Queue();
    }
    Napi::Value feed(const Napi::CallbackInfo &info)
    {
        Napi::Object data = info[0].As<Napi::Object>();
        Napi::HandleScope scope(info.Env());
        worker->feed(data);
        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value stop(const Napi::CallbackInfo &info)
    {
        worker->stop();
        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value setController(const Napi::CallbackInfo &info)
    {
        Napi::Object data = info[0].As<Napi::Object>();
        Napi::HandleScope scope(info.Env());
        int ret = worker->setController(data);
        return Napi::Number::New(info.Env(), Napi::Number::New(info.Env(), ret));
    }
    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(
            env, "H264Encoder",
            {
                InstanceMethod<&H264Encoder::feed>(
                    "feed", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                InstanceMethod<&H264Encoder::stop>(
                    "stop", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                InstanceMethod<&H264Encoder::setController>(
                    "setController", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),

            });
        *constructor = Napi::Persistent(func);
        exports.Set("H264Encoder", func);
        return exports;
    }
};
Napi::FunctionReference *H264Encoder::constructor = new Napi::FunctionReference();
#endif
#ifndef _MJPEG_ENCODER_H_
#define _MJPEG_ENCODER_H_ 1
#include "./h264_encoder.hpp"
#include <condition_variable>
#include <fcntl.h>
#include <iostream>
#include <libcamera/libcamera.h>
#include <linux/videodev2.h>
#include <mutex>
#include <napi.h>
#include <optional>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../util.hpp"

using namespace Napi;

// struct frame_data_t
// {
//     uint32_t size;
//     uint8_t *data;
// };

// using FrameType = frame_data_t *;

// struct buffer
// {
//     void *start;
//     int length;
//     struct v4l2_buffer inner;
//     struct v4l2_plane plane;
// };

// static int get_v4l2_colorspace(std::optional<libcamera::ColorSpace> const &cs)
// {
//     if (cs == libcamera::ColorSpace::Rec709)
//         return V4L2_COLORSPACE_REC709;
//     else if (cs == libcamera::ColorSpace::Smpte170m)
//         return V4L2_COLORSPACE_SMPTE170M;

//     // LOG(1, "H264: surprising colour space: " << libcamera::ColorSpace::toString(cs));
//     return V4L2_COLORSPACE_SMPTE170M;
// }

// mmaps the buffers for the given type of device (capture or output).
// void map(int fd, uint32_t type, struct buffer *buffer, bool dma)
// {
//     struct v4l2_buffer *inner = &buffer->inner;

//     memset(inner, 0, sizeof(*inner));
//     inner->type = type;
//     if (dma)
//     {
//         inner->memory = V4L2_MEMORY_DMABUF;
//     }
//     else
//     {
//         inner->memory = V4L2_MEMORY_MMAP;
//     }

//     inner->index = 0;
//     inner->length = 1;
//     inner->m.planes = &buffer->plane;
//     ioctl(fd, VIDIOC_QUERYBUF, inner);
//     // std::cout << "map plane length after query buffer: " << ((&(buffer->plane))[0]).length << std::endl;
//     buffer->length = inner->m.planes[0].length;
//     buffer->start = mmap(NULL, buffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, inner->m.planes[0].m.mem_offset);
//     if (buffer->start == (void *)-1)
//     {
//         std::cout << "mmap type: " << type << std::endl;
//         printf("mmap error : %d-%s. \r\n", errno, strerror(errno));
//     }
// }

class MjpegEncoderWorker : public AsyncProgressWorker<FrameType>
{
  public:
    uint32_t width = 640;
    uint32_t height = 480;
    uint32_t bitrate_bps = 4 * 1024 * 1024;
    int level = V4L2_MPEG_VIDEO_H264_LEVEL_4_2;
    uint32_t pixel_format = V4L2_PIX_FMT_YUYV;
    uint8_t num_planes = 1;
    struct buffer output;
    struct buffer capture;
    int fd = open("/dev/video11", O_RDWR);
    bool stopped = false;
    uint32_t total_frame = 0;
    uint32_t total_size = 0;

    MjpegEncoderWorker(Napi::Object option, Napi::Function callback) : AsyncProgressWorker(callback)
    {
        if (option.Get("width").IsNumber())
            width = option.Get("width").As<Napi::Number>().Uint32Value();
        if (option.Get("height").IsNumber())
            height = option.Get("height").As<Napi::Number>().Uint32Value();
        if (option.Get("bitrate").IsNumber())
            bitrate_bps = option.Get("bitrate").As<Napi::Number>().Uint32Value();
        if (option.Get("level").IsNumber())
            level = option.Get("level").As<Napi::Number>().Uint32Value();
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
        // 设置码率
        v4l2_control ctrl = {};
        ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
        ctrl.value = bitrate_bps;
        ioctl(fd, VIDIOC_S_CTRL, &ctrl);

        // v4l2_ext_control extCtrl={0};
        // extCtrl.id = V4L2_CID_JPEG_COMPRESSION_QUALITY;
        // extCtrl.size = 0;
        // extCtrl.value = 100;
        // v4l2_ext_controls extCtrls={0};
        // extCtrls.controls = &extCtrl;
        // extCtrls.count = 1;
        // extCtrls.ctrl_class = V4L2_CTRL_CLASS_JPEG;
        // ioctl(fd, VIDIOC_G_EXT_CTRLS, &extCtrls);
        // ctrl.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
        // ctrl.value = level;
        // ioctl(fd, VIDIOC_S_CTRL, &ctrl);

        struct v4l2_format fmt;
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        ioctl(fd, VIDIOC_G_FMT, &fmt);
        fmt.fmt.pix_mp.width = width;
        fmt.fmt.pix_mp.height = height;
        fmt.fmt.pix_mp.pixelformat = pixel_format;
        fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
        fmt.fmt.pix_mp.num_planes = num_planes;
        if (option.Get("bytesperline").IsNumber())
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = option.Get("bytesperline").As<Napi::Number>().Uint32Value();
        if (option.Get("colorspace").IsNumber())
            fmt.fmt.pix_mp.colorspace = option.Get("colorspace").As<Napi::Number>().Uint32Value();
        ioctl(fd, VIDIOC_S_FMT, &fmt);

        fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(fd, VIDIOC_G_FMT, &fmt);
        fmt.fmt.pix_mp.width = width;
        fmt.fmt.pix_mp.height = height;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
        fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
        fmt.fmt.pix_mp.num_planes = 1;
        fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 4 * 1024 * 1024;
        ioctl(fd, VIDIOC_S_FMT, &fmt);

        // struct v4l2_streamparm params;
        // memset(&params, 0, sizeof(params));
        // params.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        // params.parm.output.timeperframe.numerator = 1;
        // params.parm.output.timeperframe.denominator = 25;
        // ioctl(fd, VIDIOC_S_PARM, &params);

        struct v4l2_requestbuffers buf;
        buf.count = 1;
        // struct buffer output;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        ioctl(fd, VIDIOC_REQBUFS, &buf);

        // struct buffer capture;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_REQBUFS, &buf);
        map(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &capture, false);

        ioctl(fd, VIDIOC_QBUF, capture.inner);

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
                break;
            }
            usleep(4 * 1000);
            struct v4l2_buffer buf;
            struct v4l2_plane out_planes;
            buf.memory = V4L2_MEMORY_DMABUF;
            buf.length = 1;
            memset(&out_planes, 0, sizeof(out_planes));
            buf.m.planes = &out_planes;
            // 将output buffer出列
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            ioctl(fd, VIDIOC_DQBUF, &buf);
            // 将capture buffer出列
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.length = 1;
            memset(&out_planes, 0, sizeof(out_planes));
            buf.m.planes = &out_planes;
            ioctl(fd, VIDIOC_DQBUF, &buf);
            // 提取capture buffer里的编码数据，即H264数据
            int encoded_len = buf.m.planes[0].bytesused;
            if (encoded_len > 0)
            {
                total_frame++;
                total_size += encoded_len;
                FrameType frame_data = new frame_data_t{encoded_len, (uint8_t *)capture.start};
                progress.Send(&frame_data, sizeof(frame_data_t));
            }
            // 将capture buffer入列
            ioctl(fd, VIDIOC_QBUF, capture.inner);
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
            fast_memcpy(output.start + index, plane_data, size);
            index += size;
        }
        ioctl(fd, VIDIOC_QBUF, output.inner);
    }

    void feed(int _fd, uint32_t size)
    {
        v4l2_buffer buf = {};
        v4l2_plane planes[VIDEO_MAX_PLANES] = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.index = 0;
        buf.field = V4L2_FIELD_NONE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.length = 1;
        buf.m.planes = planes;
        buf.m.planes[0].m.fd = _fd;
        buf.m.planes[0].bytesused = size;
        buf.m.planes[0].length = size;
        ioctl(fd, VIDIOC_QBUF, &buf);
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
        std::cout << "total frame: " << total_frame << ", total size: " << total_size / 1024.0 / 1024.0 << std::endl;
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
        uint8_t *buf = (*data)->data;
        uint32_t size = (*data)->size;
        Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(Env(), buf, size, [](Napi::Env env, void *arg) {});
        Callback().Call({Env().Null(), Env().Null(), buffer});
        delete (*data);
    }
};

class MJPEGEncoder : public Napi::ObjectWrap<MJPEGEncoder>
{
  public:
    static Napi::FunctionReference *constructor;
    MjpegEncoderWorker *worker;
    MJPEGEncoder(const Napi::CallbackInfo &info) : Napi::ObjectWrap<MJPEGEncoder>(info)
    {
        Napi::Object option = info[0].As<Napi::Object>();
        Napi::Function callback = info[1].As<Napi::Function>();
        Napi::HandleScope scope(info.Env());
        worker = new MjpegEncoderWorker(option, callback);
        worker->Queue();
    }
    Napi::Value feed(const Napi::CallbackInfo &info)
    {
        Napi::Value param = info[0].As<Napi::Value>();
        if (param.IsObject())
        {
            worker->feed(param.As<Napi::Object>());
        }
        else if (param.IsNumber())
        {
            worker->feed(param.As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Uint32Value());
        }

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
        Napi::Function func = DefineClass(env, "MJPEGEncoder",
                                          {
                                              InstanceMethod<&MJPEGEncoder::feed>("feed", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&MJPEGEncoder::stop>("stop", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&MJPEGEncoder::setController>("setController", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),

                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("MJPEGEncoder", func);
        return exports;
    }
};
Napi::FunctionReference *MJPEGEncoder::constructor = new Napi::FunctionReference();
#endif
#pragma once

#ifndef _IMAGE_
#define _IMAGE_ 1
#include <libcamera/libcamera.h>
#include <napi.h>

#include "Stream.hpp"
#include "image/image.hpp"
#include "utils/util.hpp"

using WorkerType = void *;

class SaveWorker : public AsyncWorker
{
  public:
    SaveWorker(Function &callback, uint8_t _type, std::string _filename, uint32_t _frame_size, uint8_t _quality, uint8_t *_plane_data, libcamera::ControlList *_metadata, libcamera::Stream *_stream)
        : AsyncWorker(callback), type(_type), filename(_filename), frame_size(_frame_size), quality(_quality), plane_data(_plane_data), metadata(_metadata), stream(_stream)
    {
    }
    ~SaveWorker()
    {
    }
    void Execute() override
    {
        auto file_name = filename;
        auto data = libcamera::Span<uint8_t>(plane_data, frame_size);
        std::vector vec{data};
        StreamInfo stream_info;
        stream_info.width = stream->configuration().size.width;
        stream_info.height = stream->configuration().size.height;
        stream_info.stride = stream->configuration().stride;
        stream_info.pixel_format = stream->configuration().pixelFormat;
        stream_info.colour_space = stream->configuration().colorSpace;
        StillOptions *options = new StillOptions();
        if (type == 1)
        {
            dng_save(vec, stream_info, *metadata, file_name, "picam", options);
        }
        if (type == 2)
        {
            options->quality = quality;
            jpeg_save(vec, stream_info, *metadata, file_name, "picam", options);
        }
        if (type == 3)
        {
            yuv_save(vec, stream_info, file_name, nullptr);
        }
        if (type == 4)
        {
            qoi_save(vec, stream_info, file_name, nullptr);
        }
        delete[] plane_data;
        delete options;
    }
    void OnOK() override
    {
        HandleScope scope(Env());
        Callback().Call({Env().Null()});
    }

  private:
    uint8_t type;
    uint8_t quality;
    uint8_t *plane_data;
    libcamera::ControlList *metadata;
    std::string filename;
    uint32_t frame_size;
    libcamera::Stream *stream;
};

class Image : public Napi::ObjectWrap<Image>
{
  public:
    static Napi::FunctionReference *constructor;
    // libcamera::Camera *camera;
    libcamera::Request *request;
    int fd;
    uint32_t frame_size;
    libcamera::Stream *stream;
    libcamera::FrameBuffer *buffer;
    // stream_config *s_config;
    libcamera::ControlList *metadata;
    // uint8_t data_format = 0;
    void *memory = nullptr;
    uint8_t *data = nullptr;

    Image(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Image>(info)
    {
        bool lossless = true;
        // camera = reinterpret_cast<libcamera::Camera *>(info[0].As<Napi::BigInt>().Int64Value(&lossless));
        stream = reinterpret_cast<libcamera::Stream *>(info[0].As<Napi::BigInt>().Int64Value(&lossless));
        request = reinterpret_cast<libcamera::Request *>(info[1].As<Napi::BigInt>().Int64Value(&lossless));
        // data_format = info[2].As<Napi::Number>().Uint32Value();
        metadata = new libcamera::ControlList(request->metadata());
        // request status is COMPLETE
        const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> &buffers = request->buffers();
        buffer = buffers.at(stream);
        frame_size = stream->configuration().frameSize;
        auto plane = buffer->planes()[0];
        fd = plane.fd.get();
    }

    ~Image()
    {
        delete metadata;
        delete[] data;
    }

    Napi::Value getFd(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), fd);
    }

    Napi::Value getFrameSize(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), frame_size);
    }

    Napi::Value getData(const Napi::CallbackInfo &info)
    {
        if (memory != nullptr && data != nullptr)
        {
            Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), data, stream->configuration().frameSize, [](Napi::Env env, void *arg) {});
            return buf;
        }

        memory = mmap(NULL, stream->configuration().frameSize, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->planes()[0].fd.get(), 0);
        data = new uint8_t[stream->configuration().frameSize];
        if (memory == (void *)-1)
        {
            printf("mmap error : %d-%s. \r\n", errno, strerror(errno));
        }
        fast_memcpy(data, memory, stream->configuration().frameSize);
        Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), data, stream->configuration().frameSize, [](Napi::Env env, void *arg) {});
        return buf;
    }

    Napi::Value getColorSpace(const Napi::CallbackInfo &info)
    {
        return Napi::String::New(info.Env(), stream->configuration().colorSpace->toString());
    }

    Napi::Value getStride(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream->configuration().stride);
    }

    Napi::Value getPixelFormat(const Napi::CallbackInfo &info)
    {
        return Napi::String::New(info.Env(), stream->configuration().pixelFormat.toString());
    }

    Napi::Value getPixelFormatFourcc(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream->configuration().pixelFormat.fourcc());
    }

    Napi::Value save(const Napi::CallbackInfo &info)
    {
        auto option = info[0].As<Napi::Object>();
        auto file_name = option.Get("file").As<Napi::String>().ToString();
        auto type = option.Get("type").As<Napi::Number>().Uint32Value();
        uint8_t quality = 90;
        if (option.Has("quality") && option.Get("quality").IsNumber())
            quality = option.Get("quality").As<Napi::Number>().Uint32Value();
        Function cb = info[1].As<Function>();
        auto plane = buffer->planes()[0];
        void *memory = mmap(NULL, frame_size, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
        uint8_t *copy_mem = new uint8_t[frame_size];
        fast_memcpy(copy_mem, memory, frame_size);
        auto wk = new SaveWorker(cb, type, file_name, frame_size, quality, copy_mem, metadata, stream);
        wk->Queue();
        return info.Env().Undefined();
    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "Image",
                                          {
                                              InstanceAccessor<&Image::getFd>("fd", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getFrameSize>("frameSize", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Image::getData>("getData", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getColorSpace>("colorSpace", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getStride>("stride", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getPixelFormat>("pixelFormat", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getPixelFormatFourcc>("pixelFormatFourcc", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Image::save>("save", static_cast<napi_property_attributes>(napi_enumerable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Image", func);
        return exports;
    }
};

Napi::FunctionReference *Image::constructor = new Napi::FunctionReference();
#endif
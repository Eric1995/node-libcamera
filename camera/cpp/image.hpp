#pragma once

#ifndef _IMAGE_
#define _IMAGE_ 1
#include <cstdint>
#include <libcamera/libcamera.h>
#include <map>
#include <napi.h>
#include <stdint.h>
#include <string>
#include <sys/mman.h>

// #include "dng_writer.h"
#include "image/image.hpp"
#include "stream.hpp"
#include "util.hpp"

using WorkerType = void *;

class SaveWorker : public AsyncWorker
{
  public:
    SaveWorker(Function &callback, uint8_t _type, std::string _filename, uint32_t _frame_size, libcamera::FrameBuffer *_buffer, libcamera::ControlList *_metadata, libcamera::Stream *_stream)
        : AsyncWorker(callback), type(_type), filename(_filename), frame_size(_frame_size), buffer(_buffer), metadata(_metadata), stream(_stream)
    {
    }
    ~SaveWorker()
    {
    }
    void Execute() override
    {
        auto file_name = filename;
        auto plane = buffer->planes()[0];

        void *memory = mmap(NULL, frame_size, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
        auto data = libcamera::Span<uint8_t>(static_cast<uint8_t *>(memory), frame_size);
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
            options->quality = 90;
            jpeg_save(vec, stream_info, *metadata, file_name, "picam", options);
        }
        if (type == 3)
        {
            yuv_save(vec, stream_info, file_name, nullptr);
        }
        delete options;
    }
    void OnOK() override
    {
        HandleScope scope(Env());
        Callback().Call({Env().Null()});
    }

  private:
    uint8_t type;
    libcamera::FrameBuffer *buffer;
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
    stream_config *s_config;
    libcamera::ControlList *metadata;
    uint8_t data_format = 0;

    Image(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Image>(info)
    {
        bool lossless = true;
        // camera = reinterpret_cast<libcamera::Camera *>(info[0].As<Napi::BigInt>().Int64Value(&lossless));
        stream = reinterpret_cast<libcamera::Stream *>(info[0].As<Napi::BigInt>().Int64Value(&lossless));
        request = reinterpret_cast<libcamera::Request *>(info[1].As<Napi::BigInt>().Int64Value(&lossless));
        data_format = info[2].As<Napi::Number>().Uint32Value();
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
        if (s_config->data_format == 0)
        {
            uint8_t *data = new uint8_t[stream->configuration().frameSize];
            void *memory = mmap(NULL, stream->configuration().frameSize, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->planes()[0].fd.get(), 0);
            if (memory == (void *)-1)
            {
                printf("mmap error : %d-%s. \r\n", errno, strerror(errno));
            }
            fast_memcpy(data, memory, stream->configuration().frameSize);
            Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), data, stream->configuration().frameSize, [](Napi::Env env, void *arg) { delete[] arg; });
            return buf;
        }
        return info.Env().Undefined();
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

    Napi::Value toDNG(const Napi::CallbackInfo &info)
    {
        auto file_name = info[0].As<Napi::String>().Utf8Value();
        Function cb = info[1].As<Function>();
        auto wk = new SaveWorker(cb, 1, file_name, frame_size, buffer, metadata, stream);
        wk->Queue();
        // auto plane = buffer->planes()[0];

        // void *memory = mmap(NULL, frame_size, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
        // auto data = libcamera::Span<uint8_t>(static_cast<uint8_t *>(memory), frame_size);
        // std::vector vec{data};
        // StreamInfo stream_info;
        // stream_info.width = stream->configuration().size.width;
        // stream_info.height = stream->configuration().size.height;
        // stream_info.stride = stream->configuration().stride;
        // stream_info.pixel_format = stream->configuration().pixelFormat;
        // stream_info.colour_space = stream->configuration().colorSpace;
        // dng_save(vec, stream_info, *metadata, file_name, "picam", nullptr);

        // void *memory = mmap(NULL, plane.length, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
        // DNGWriter::write(file_name.c_str(), camera, stream->configuration(), request->controls(), buffer, memory);
        // return Napi::Number::New(info.Env(), frame_size);
        return info.Env().Undefined();
    }

    Napi::Value toJPEG(const Napi::CallbackInfo &info)
    {
        auto file_name = info[0].As<Napi::String>().Utf8Value();
        Function cb = info[1].As<Function>();
        auto wk = new SaveWorker(cb, 2, file_name, frame_size, buffer, metadata, stream);
        wk->Queue();
        return info.Env().Undefined();
    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "Image",
                                          {
                                              InstanceAccessor<&Image::getFd>("fd", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getFrameSize>("frameSize", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getData>("data", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getColorSpace>("colorSpace", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getStride>("stride", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getPixelFormat>("pixelFormat", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Image::getPixelFormatFourcc>("pixelFormatFourcc", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Image::toDNG>("toDNG", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Image::toJPEG>("toJPEG", static_cast<napi_property_attributes>(napi_enumerable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Image", func);
        return exports;
    }
};

Napi::FunctionReference *Image::constructor = new Napi::FunctionReference();
#endif
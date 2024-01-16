#ifndef _STREAM_H_
#define _STREAM_H_ 1
#include <cstdint>
#include <libcamera/libcamera.h>
#include <map>
#include <napi.h>
#include <stdint.h>
#include <string>
#include <sys/mman.h>

using namespace Napi;

struct stream_config
{
    bool auto_queue_request = false;
    uint8_t data_format = 1;
    FunctionReference callback_ref;
};

class Stream : public Napi::ObjectWrap<Stream>
{
  public:
    static Napi::FunctionReference *constructor;
    uint32_t index = 0;
    libcamera::Camera *camera;
    libcamera::CameraConfiguration *camera_config;
    // std::deque<std::unique_ptr<libcamera::Request>> *requests;
    libcamera::StreamConfiguration *stream_configuration;
    std::map<libcamera::Stream *, stream_config *> *stream_config_map;

    Stream(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Stream>(info)
    {
        bool lossless = true;
        index = info[0].As<Napi::Number>().Uint32Value();
        camera = reinterpret_cast<libcamera::Camera *>(info[1].As<Napi::BigInt>().Int64Value(&lossless));
        camera_config = reinterpret_cast<libcamera::CameraConfiguration *>(info[2].As<Napi::BigInt>().Int64Value(&lossless));
        // requests = reinterpret_cast<std::deque<std::unique_ptr<libcamera::Request>> *>(info[3].As<Napi::BigInt>().Int64Value(&lossless));
        stream_configuration = reinterpret_cast<libcamera::StreamConfiguration *>(info[3].As<Napi::BigInt>().Int64Value(&lossless));
        stream_config_map = reinterpret_cast<std::map<libcamera::Stream *, stream_config *> *>(info[4].As<Napi::BigInt>().Int64Value(&lossless));
    }

    Napi::Value configStream(const Napi::CallbackInfo &info)
    {
        Napi::Object option = info[0].As<Napi::Object>();
        auto stream = stream_configuration->stream();
        auto _config = (*stream_config_map)[stream];
        if (option.Has("onImageData") && option.Get("onImageData").IsFunction())
        {
            auto onImageData = option.Get("onImageData").As<Napi::Function>();
            _config->callback_ref = Napi::Persistent(onImageData);
        }
        if (option.Has("auto_queue_request") && option.Get("auto_queue_request").IsBoolean())
        {
            _config->auto_queue_request = option.Get("auto_queue_request").As<Napi::Boolean>();
        }
        if (option.Has("data_output_type") && option.Get("data_output_type").IsNumber())
        {
            _config->data_format = option.Get("data_output_type").As<Napi::Number>().Uint32Value();
        }

        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value getStride(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream_configuration->stride);
    }

    Napi::Value getColorSpace(const Napi::CallbackInfo &info)
    {
        return Napi::String::New(info.Env(), stream_configuration->colorSpace->toString());
    }
    Napi::Value getPixelFormat(const Napi::CallbackInfo &info)
    {
        return Napi::String::New(info.Env(), stream_configuration->pixelFormat.toString());
    }
    Napi::Value getPixelFormatcc(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream_configuration->pixelFormat.fourcc());
    }
    Napi::Value getFrameSize(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream_configuration->frameSize);
    }
    Napi::Value getWidth(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream_configuration->size.width);
    }
    Napi::Value getHeight(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stream_configuration->size.height);
    }
    Napi::Value getStreamIndex(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), index);
    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "Stream",
                                          {
                                              InstanceAccessor<&Stream::getStride>("stride"),
                                              InstanceAccessor<&Stream::getColorSpace>("colorSpace"),
                                              InstanceAccessor<&Stream::getPixelFormat>("pixelFormat"),
                                              InstanceAccessor<&Stream::getPixelFormatcc>("pixelFormtFourcc"),
                                              InstanceAccessor<&Stream::getFrameSize>("frameSize"),
                                              InstanceAccessor<&Stream::getWidth>("width"),
                                              InstanceAccessor<&Stream::getHeight>("height"),
                                              InstanceAccessor<&Stream::getStreamIndex>("streamIndex"),
                                              InstanceMethod<&Stream::configStream>("configStream", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Stream", func);
        return exports;
    }
};

Napi::FunctionReference *Stream::constructor = new Napi::FunctionReference();
#endif
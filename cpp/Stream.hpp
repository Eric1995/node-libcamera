#ifndef _STREAM_H_
#define _STREAM_H_ 1

#include <libcamera/libcamera.h>
#include <napi.h>
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
  private:
  public:
    static Napi::FunctionReference *constructor;
    int index = -1;
    // libcamera::Camera *camera;

    bool auto_queue_request = false;
    uint8_t data_format = 1;
    FunctionReference *callback_ref;

    unsigned int stride;
    std::optional<libcamera::ColorSpace> colorSpace;
    std::string pixelFormat;
    unsigned int frameSize;
    libcamera::Size *size;

    // libcamera::CameraConfiguration *camera_config;
    // std::deque<std::unique_ptr<libcamera::Request>> *requests;
    libcamera::StreamConfiguration *stream_configuration;
    static std::map<unsigned int, stream_config *> stream_config_map;

    stream_config *config;

    Stream(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Stream>(info)
    {
        // bool lossless = true;
        index = info[0].As<Napi::Number>().Uint32Value();
        stride = info[1].As<Napi::Number>().Uint32Value();
        colorSpace = libcamera::ColorSpace::fromString(info[2].As<Napi::String>());
        frameSize = info[3].As<Napi::Number>().Uint32Value();
        uint32_t width = info[4].As<Napi::Number>().Uint32Value();
        uint32_t height = info[5].As<Napi::Number>().Uint32Value();
        size = new libcamera::Size(width, height);
        pixelFormat = info[6].As<Napi::String>();
        config = new stream_config();
        // camera = reinterpret_cast<libcamera::Camera *>(info[1].As<Napi::BigInt>().Int64Value(&lossless));
        // camera_config = reinterpret_cast<libcamera::CameraConfiguration *>(info[2].As<Napi::BigInt>().Int64Value(&lossless));
        // requests = reinterpret_cast<std::deque<std::unique_ptr<libcamera::Request>> *>(info[3].As<Napi::BigInt>().Int64Value(&lossless));
        // stream_configuration = reinterpret_cast<libcamera::StreamConfiguration *>(info[3].As<Napi::BigInt>().Int64Value(&lossless));
        // stream_config_map = reinterpret_cast<std::map<libcamera::Stream *, stream_config *> *>(info[4].As<Napi::BigInt>().Int64Value(&lossless));
    }
    ~Stream()
    {
        delete size;
    }
    Napi::Value configStream(const Napi::CallbackInfo &info)
    {
        Napi::Object option = info[0].As<Napi::Object>();
        // auto stream = stream_configuration->stream();
        // auto _config = (*stream_config_map)[stream];
        if (option.Has("onImageData") && option.Get("onImageData").IsFunction())
        {
            auto onImageData = option.Get("onImageData").As<Napi::Function>();
            std::cout << "store on image data callback" << std::endl;
            // FunctionReference ref = Napi::Persistent(onImageData);
            // callback_ref = new FunctionReference(Napi::Persistent(onImageData));
            // config->callback_ref = Napi::Persistent(onImageData);
            stream_config_map[index]->callback_ref = Napi::Persistent(onImageData);
        }
        if (option.Has("auto_queue_request") && option.Get("auto_queue_request").IsBoolean())
        {
            auto_queue_request = option.Get("auto_queue_request").As<Napi::Boolean>();
        }
        if (option.Has("data_output_type") && option.Get("data_output_type").IsNumber())
        {
            data_format = option.Get("data_output_type").As<Napi::Number>().Uint32Value();
            stream_config_map[index]->data_format = data_format;
        }

        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value getStride(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), stride);
    }

    Napi::Value getColorSpace(const Napi::CallbackInfo &info)
    {
        return Napi::String::New(info.Env(), colorSpace->toString());
    }
    Napi::Value getPixelFormat(const Napi::CallbackInfo &info)
    {
        return Napi::String::New(info.Env(), libcamera::PixelFormat::fromString(pixelFormat).toString());
    }
    Napi::Value getPixelFormatcc(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), libcamera::PixelFormat::fromString(pixelFormat).fourcc());
    }
    Napi::Value getFrameSize(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), frameSize);
    }
    Napi::Value getWidth(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), size->width);
    }
    Napi::Value getHeight(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), size->height);
    }
    Napi::Value getStreamIndex(const Napi::CallbackInfo &info)
    {
        return Napi::Number::New(info.Env(), index);
    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "Stream",
                                          {
                                              InstanceAccessor<&Stream::getStride>("stride", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getColorSpace>("colorSpace", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getPixelFormat>("pixelFormat", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getPixelFormatcc>("pixelFormtFourcc", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getFrameSize>("frameSize", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getWidth>("width", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getHeight>("height", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceAccessor<&Stream::getStreamIndex>("streamIndex", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Stream::configStream>("configStream", static_cast<napi_property_attributes>(napi_writable | napi_configurable | napi_enumerable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Stream", func);
        return exports;
    }
};
std::map<unsigned int, stream_config *> Stream::stream_config_map;
Napi::FunctionReference *Stream::constructor = new Napi::FunctionReference();
#endif
#include "Stream.h"

// std::map<unsigned int, stream_config *> Stream::stream_config_map;

Stream::Stream(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Stream>(info)
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
Stream::~Stream()
{
    delete size;
}
Napi::Value Stream::configStream(const Napi::CallbackInfo &info)
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

Napi::Value Stream::getStride(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), stride);
}

Napi::Value Stream::getColorSpace(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), colorSpace->toString());
}
Napi::Value Stream::getPixelFormat(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), libcamera::PixelFormat::fromString(pixelFormat).toString());
}
Napi::Value Stream::getPixelFormatcc(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), libcamera::PixelFormat::fromString(pixelFormat).fourcc());
}
Napi::Value Stream::getFrameSize(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), frameSize);
}
Napi::Value Stream::getWidth(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), size->width);
}
Napi::Value Stream::getHeight(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), size->height);
}
Napi::Value Stream::getStreamIndex(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), index);
}

Napi::Object Stream::Init(Napi::Env env, Napi::Object exports)
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
std::map<unsigned int, stream_config *> Stream::stream_config_map;
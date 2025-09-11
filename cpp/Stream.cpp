#include "Stream.h"

Stream::Stream(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Stream>(info)
{
    index = info[0].As<Napi::Number>().Uint32Value();
    streamConfiguration = *info[1].As<Napi::External<libcamera::StreamConfiguration>>().Data();
    config = new stream_config();
}
Stream::~Stream()
{
    // The stream_config is managed by Camera::clean()
    // delete config;
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
    return Napi::Number::New(info.Env(), streamConfiguration.stride);
}

Napi::Value Stream::getColorSpace(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), streamConfiguration.colorSpace->toString());
}
Napi::Value Stream::getPixelFormat(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), streamConfiguration.pixelFormat.toString());
}
Napi::Value Stream::getPixelFormatcc(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration.pixelFormat.fourcc());
}
Napi::Value Stream::getFrameSize(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration.frameSize);
}
Napi::Value Stream::getWidth(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration.size.width);
}
Napi::Value Stream::getHeight(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration.size.height);
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
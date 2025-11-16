#include "Stream.h"

Stream::Stream(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Stream>(info)
{
    index = info[0].As<Napi::Number>().Uint32Value();
    streamConfiguration = info[1].As<Napi::External<libcamera::StreamConfiguration>>().Data();

    // 从 JS 传入的回调函数
    if (info.Length() > 2 && info[2].IsFunction())
    {
        Napi::Function jsFunc = info[2].As<Napi::Function>();
        callback_ref = Napi::Persistent(jsFunc);
    }
}

Stream::~Stream()
{
    callback_ref.Reset();
}

Napi::Value Stream::configStream(const Napi::CallbackInfo &info)
{
    Napi::Object option = info[0].As<Napi::Object>();

    if (option.Has("onImageData") && option.Get("onImageData").IsFunction())
    {
        Napi::Function onImageData = option.Get("onImageData").As<Napi::Function>();
        // 直接更新当前 Stream 实例的回调
        callback_ref.Reset(); // 释放旧的回调
        callback_ref = Napi::Persistent(onImageData);
    }

    return Napi::Number::New(info.Env(), 0);
}

Napi::Value Stream::getStride(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration->stride);
}

Napi::Value Stream::getColorSpace(const Napi::CallbackInfo &info)
{
    if (streamConfiguration->colorSpace.has_value())
    {
        return Napi::String::New(info.Env(), streamConfiguration->colorSpace->toString());
    }
    return info.Env().Null();
}

Napi::Value Stream::getPixelFormat(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), streamConfiguration->pixelFormat.toString());
}

Napi::Value Stream::getPixelFormatcc(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration->pixelFormat.fourcc());
}

Napi::Value Stream::getFrameSize(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration->frameSize);
}

Napi::Value Stream::getWidth(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration->size.width);
}

Napi::Value Stream::getHeight(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), streamConfiguration->size.height);
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
    constructor = Napi::Persistent(func);
    exports.Set("Stream", func);
    return exports;
}

Napi::FunctionReference Stream::constructor;

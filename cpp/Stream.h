#pragma once

#include <libcamera/libcamera.h>
#include <napi.h>
#include <sys/mman.h>

using namespace Napi;

class Stream : public Napi::ObjectWrap<Stream>
{
  private:
  public:
    static Napi::FunctionReference constructor;
    int index = -1;

    bool auto_queue_request = false;
    uint8_t data_format = 1;
    FunctionReference callback_ref;

    libcamera::StreamConfiguration *streamConfiguration;
    Stream(const Napi::CallbackInfo &info);
    ~Stream();
    Napi::Value configStream(const Napi::CallbackInfo &info);
    Napi::Value getStride(const Napi::CallbackInfo &info);
    Napi::Value getColorSpace(const Napi::CallbackInfo &info);
    Napi::Value getPixelFormat(const Napi::CallbackInfo &info);
    Napi::Value getPixelFormatcc(const Napi::CallbackInfo &info);
    Napi::Value getFrameSize(const Napi::CallbackInfo &info);
    Napi::Value getWidth(const Napi::CallbackInfo &info);
    Napi::Value getHeight(const Napi::CallbackInfo &info);
    Napi::Value getStreamIndex(const Napi::CallbackInfo &info);
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
};
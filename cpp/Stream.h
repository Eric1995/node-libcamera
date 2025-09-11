#pragma once

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

    // unsigned int stride;
    // std::optional<libcamera::ColorSpace> colorSpace;
    // std::string pixelFormat;
    // unsigned int frameSize;
    // libcamera::Size *size;

    // libcamera::CameraConfiguration *camera_config;
    // std::deque<std::unique_ptr<libcamera::Request>> *requests;
    libcamera::StreamConfiguration streamConfiguration;
    static std::map<unsigned int, stream_config *> stream_config_map;

    stream_config *config;
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
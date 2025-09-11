#pragma once

#include <libcamera/libcamera.h>
#include <napi.h>

#include "Stream.h"
#include "image/image.h"
#include "utils/util.h"

using WorkerType = void *;

class SaveWorker : public AsyncWorker
{
  public:
    SaveWorker(Function &callback, uint8_t _type, std::string _filename, uint32_t _frame_size, uint8_t _quality, uint8_t *_plane_data, libcamera::ControlList *_metadata, libcamera::Stream *_stream);

    ~SaveWorker();
    void Execute() override;

    void OnOK() override;

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

    Image(const Napi::CallbackInfo &info);

    ~Image();

    Napi::Value getFd(const Napi::CallbackInfo &info);

    Napi::Value getFrameSize(const Napi::CallbackInfo &info);

    Napi::Value getData(const Napi::CallbackInfo &info);

    Napi::Value getColorSpace(const Napi::CallbackInfo &info);

    Napi::Value getStride(const Napi::CallbackInfo &info);

    Napi::Value getPixelFormat(const Napi::CallbackInfo &info);

    Napi::Value getPixelFormatFourcc(const Napi::CallbackInfo &info);

    Napi::Value save(const Napi::CallbackInfo &info);

    static Napi::Object Init(Napi::Env env, Napi::Object exports);
};

// Napi::FunctionReference *Image::constructor = new Napi::FunctionReference();
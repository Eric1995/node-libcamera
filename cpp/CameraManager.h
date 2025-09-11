#pragma once
#include <libcamera/libcamera.h>
#include <memory>
#include <napi.h>

#include "Camera.h"

class CameraManager : public Napi::ObjectWrap<CameraManager>
{
  public:
    std::shared_ptr<libcamera::CameraManager> cm;
    static Napi::FunctionReference *constructor;
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    CameraManager(const Napi::CallbackInfo &info);
    static Napi::Value CreateNewItem(const Napi::CallbackInfo &info);
    std::vector<Napi::Reference<Napi::Object>> cameras;

  private:
    Napi::Value GetCameras(const Napi::CallbackInfo &info);
};

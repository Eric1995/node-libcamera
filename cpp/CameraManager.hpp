#ifndef _CAMERA_MANAGER_H_
#define _CAMERA_MANAGER_H_ 1
#include <libcamera/libcamera.h>
#include <memory>
#include <napi.h>

#include "Camera.hpp"

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
Napi::FunctionReference *CameraManager::constructor = new Napi::FunctionReference();

Napi::Object CameraManager::Init(Napi::Env env, Napi::Object exports)
{
    // This method is used to hook the accessor and method callbacks
    Napi::Function func = DefineClass(env, "CameraManager",
                                      {
                                          InstanceAccessor<&CameraManager::GetCameras>("cameras"),
                                          StaticMethod<&CameraManager::CreateNewItem>("CreateNewItem", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                      });

    // Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    exports.Set("CameraManager", func);
    // env.SetInstanceData<Napi::FunctionReference>(constructor);
    return exports;
}

CameraManager::CameraManager(const Napi::CallbackInfo &info) : Napi::ObjectWrap<CameraManager>(info)
{
    Napi::Env env = info.Env();
    // ...
    // Napi::Number value = info[0].As<Napi::Number>();
    // this->_value = value.DoubleValue();
    this->cm = std::make_shared<libcamera::CameraManager>();
    this->cm->start();
    auto libcameras = this->cm->cameras();
    for (auto &libcam : libcameras)
    {
        auto external_cam_mgr = Napi::External<std::shared_ptr<libcamera::CameraManager>>::New(env, &this->cm);
        auto external_cam = Napi::External<std::shared_ptr<libcamera::Camera>>::New(env, &libcam);
        auto cam = Camera::constructor->New({external_cam_mgr, external_cam});
        auto objRef = Napi::ObjectReference::New(cam);
        cameras.push_back(std::move(objRef));
    }
}

Napi::Value CameraManager::GetCameras(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Array _cameras = Napi::Array::New(info.Env());
    // _cameras[1] = Camera::constructor->New({});
    for (int i = 0; i < cameras.size(); i++)
    {
        // cameras.pop_back();
        _cameras[i] = cameras[i].Value();
    }
    return _cameras;
}

Napi::Value CameraManager::CreateNewItem(const Napi::CallbackInfo &info)
{
    //   Napi::FunctionReference* constructor =
    //       info.Env().GetInstanceData<Napi::FunctionReference>();
    return constructor->New({});
}

#endif

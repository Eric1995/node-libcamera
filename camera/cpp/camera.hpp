#ifndef _CAMERA_H_
#define _CAMERA_H_ 1
#include "camera_worker.hpp"
#include "util.hpp"
#include <cstdint>
#include <map>
#include <napi.h>
#include <stdint.h>
#include <string>
#include <sys/mman.h>

libcamera::CameraManager cm;

class Camera : public Napi::ObjectWrap<Camera>
{
  public:
    static Napi::FunctionReference *constructor;
    CameraWorker *worker = nullptr;
    uint32_t num = 0;
    std::shared_ptr<libcamera::Camera> camera;
    bool queued = false;
    bool released = false;

    Camera(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Camera>(info)
    {
        auto num = info[0].As<Napi::Number>().Uint32Value();
        this->num = num;
        auto libcameras = cm.cameras();
        auto camera = libcameras[num];
        this->camera = camera;
        worker = new CameraWorker(info, camera);
        worker->SuppressDestruct();
    }

    ~Camera()
    {
        delete this->worker;
    }

    Napi::Value getId(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        auto libcameras = cm.cameras();
        return Napi::String::New(env, camera->id());
    };

    Napi::Value basicInfo(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        Napi::Object a = Napi::Object::New(env);
        a.Set("id", camera->id());
        auto area = camera->properties().get(libcamera::properties::PixelArrayActiveAreas);
        a.Set("width", (*area)[0].size().width);
        a.Set("height", (*area)[0].size().height);
        return a;
    };

    Napi::Value createStreams(const Napi::CallbackInfo &info)
    {
        Napi::HandleScope scope(info.Env());
        auto list = worker->createStreams(info);
        return list;
        auto configs = worker->getStreamConfig();
        Napi::Array ret = Napi::Array::New(info.Env(), configs.size());
        uint32_t i = 0;
        for (auto config : configs)
        {
            Napi::Object obj = Napi::Object::New(info.Env());
            obj.Set("stride", config.stride);
            obj.Set("colorSpace", config.colorSpace->toString());
            obj.Set("pixelFormat", config.pixelFormat.toString());
            obj.Set("pixelFormtFourcc", config.pixelFormat.fourcc());
            obj.Set("frameSize", config.frameSize);
            obj.Set("width", config.size.width);
            obj.Set("height", config.size.height);
            obj.Set("streamIndex", i);
            ret[i] = obj;
            i++;
        }
        return ret;
    }

    Napi::Value config(const Napi::CallbackInfo &info)
    {
        auto option = info[0].As<Napi::Object>();
        if (option.Get("autoQueueRequest").IsBoolean())
            worker->auto_queue_request = option.Get("autoQueueRequest").As<Napi::Boolean>();
        if (option.Get("maxFrameRate").IsNumber())
            worker->setMaxFrameRate(option.Get("maxFrameRate").As<Napi::Number>().FloatValue());
        if (option.Get("onImageData").IsFunction())
            worker->callback_ref = Napi::Persistent(option.Get("onImageData").As<Napi::Function>());
        if (option.Get("crop").IsObject())
        {
            auto crop = option.Get("crop").As<Napi::Object>();
            worker->setCrop(crop.Get("roi_x").As<Napi::Number>().FloatValue(), crop.Get("roi_y").As<Napi::Number>().FloatValue(), crop.Get("roi_width").As<Napi::Number>().FloatValue(),
                            crop.Get("roi_height").As<Napi::Number>().FloatValue());
        }
        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value start(const Napi::CallbackInfo &info)
    {
        std::cout << "start camera" << std::endl;
        auto ret = worker->start();
        if (!queued)
        {
            std::cout << "queue worker" << std::endl;
            worker->Queue();
            queued = true;
        }

        return Napi::Number::New(info.Env(), ret);
    }

    Napi::Value queueRequest(const Napi::CallbackInfo &info)
    {
        worker->queueRequest();
        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value getAvailableControls(const Napi::CallbackInfo &info)
    {
        auto control_map = camera->controls();
        Napi::Object obj = Napi::Object::New(info.Env());
        for (auto const &[id, info] : control_map)
        {
            obj.Set(id->name(), info.toString());
        }
        return obj;
    }

    Napi::Value stop(const Napi::CallbackInfo &info)
    {
        if (worker)
        {
            worker->StopCamera();
        }
        // worker = new CameraWorker(info, camera);
        // worker->SuppressDestruct();

        return Napi::Boolean::New(info.Env(), true);
    }

    Napi::Value release(const Napi::CallbackInfo &info)
    {
        if (worker)
        {
            worker->Release();
        }
        // worker = new CameraWorker(info, camera);
        // worker->SuppressDestruct();

        return Napi::Boolean::New(info.Env(), true);
    }

    Napi::Value setMaxFrameRate(const Napi::CallbackInfo &info)
    {
        auto rate = info[0].As<Napi::Number>().FloatValue();
        worker->setMaxFrameRate(rate);
        return Napi::Boolean::New(info.Env(), true);
    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "Camera",
                                          {
                                              InstanceAccessor<&Camera::getId>("id"),
                                              InstanceMethod<&Camera::basicInfo>("getInfo", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::start>("start", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::queueRequest>("queueRequest", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::config>("config", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::stop>("stop", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::createStreams>("createStreams", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::getAvailableControls>("getAvailableControls", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::setMaxFrameRate>("setMaxFrameRate", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::release>("release", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Camera", func);
        return exports;
    }
};

Napi::FunctionReference *Camera::constructor = new Napi::FunctionReference();
#endif
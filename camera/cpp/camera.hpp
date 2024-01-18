#ifndef _CAMERA_H_
#define _CAMERA_H_ 1

#include "dma_heaps.hpp"
#include "echo_worker.hpp"
#include "util.hpp"
#include <cstdint>
#include <deque>
#include <iostream>
#include <libcamera/libcamera.h>
#include <map>
#include <napi.h>
#include <stdint.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

libcamera::CameraManager cm;

struct crop_t
{
    float roi_x = 0;
    float roi_y = 0;
    float roi_width = 1;
    float roi_height = 1;
};

class Camera : public Napi::ObjectWrap<Camera>
{
  private:
    EchoWorker *worker = nullptr;
    enum Status
    {
        Available,
        Acquired,
        Configured,
        Stopping,
        Running
    };
    Status state = Available;
    uint32_t num = 0;
    std::shared_ptr<libcamera::Camera> camera;
    bool queued = false;
    bool released = false;

    /** 是否自动发送request请求*/
    bool auto_queue_request = true;
    float max_frame_rate = 30.0;
    crop_t crop;

    std::vector<libcamera::StreamRole> stream_roles;
    std::unique_ptr<libcamera::CameraConfiguration> camera_config = nullptr;
    libcamera::ControlList control_list;
    std::deque<libcamera::Stream *> streams;
    std::vector<std::unique_ptr<libcamera::Request>> requests;
    std::deque<libcamera::Request *> requests_deque;
    std::deque<libcamera::Request *> wait_deque;
    std::map<libcamera::Stream *, Stream *> napi_stream_map;
    std::map<libcamera::Stream *, unsigned int> stream_index_map;

    DmaHeap dma_heap_;

  public:
    static Napi::FunctionReference *constructor;

    Camera(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Camera>(info)
    {
        auto num = info[0].As<Napi::Number>().Uint32Value();
        this->num = num;
        auto libcameras = cm.cameras();
        auto camera = libcameras[num];
        this->camera = camera;
    }

    ~Camera()
    {
        if (state != Available)
        {
            camera->stop();
            camera->release();
        }
        clean();
        std::cout << "camera destroyed" << std::endl;
    }

    void clean()
    {
        Stream::stream_config_map.clear();
        streams.clear();
        stream_index_map.clear();
        for (auto &request : requests)
        {
            for (auto const &buffer_map : request->buffers())
            {
                delete buffer_map.second;
            }
            // request->buffers().clear();
        }
        requests.clear();

        requests_deque.clear();
        wait_deque.clear();
        camera->requestCompleted.disconnect();
    }

    Napi::Value getId(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
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
        clean();
        Napi::HandleScope scope(info.Env());
        stream_roles.clear();
        Napi::Array optionList = info[0].As<Napi::Array>();
        for (int i = 0; i < optionList.Length(); i++)
        {
            Napi::Object option = optionList.Get(i).As<Napi::Object>();
            if (option.Has("role"))
            {
                auto roleValue = option.Get("role");
                if (roleValue.IsString())
                {
                    auto role = roleValue.As<Napi::String>().Utf8Value();
                    if (role == "Raw")
                    {
                        stream_roles.push_back(libcamera::StreamRole::Raw);
                    }
                    if (role == "VideoRecording")
                    {
                        stream_roles.push_back(libcamera::StreamRole::VideoRecording);
                    }
                    if (role == "StillCapture")
                    {
                        stream_roles.push_back(libcamera::StreamRole::StillCapture);
                    }
                    if (role == "Viewfinder")
                    {
                        stream_roles.push_back(libcamera::StreamRole::Viewfinder);
                    }
                }
            }
        }
        if (state == Available)
        {
            camera->acquire();
            state = Acquired;
        }
        auto config = camera->generateConfiguration(stream_roles);
        camera_config = std::move(config);
        for (int i = 0; i < optionList.Length(); i++)
        {
            Napi::Object option = optionList.Get(i).As<Napi::Object>();
            libcamera::StreamConfiguration &streamConfig = camera_config->at(i);
            if (option.Has("width") && option.Get("width"))
                streamConfig.size.width = option.Get("width").As<Napi::Number>().Uint32Value();
            if (option.Has("height") && option.Get("height").IsNumber())
                streamConfig.size.height = option.Get("height").As<Napi::Number>().Uint32Value();
            if (option.Has("pixel_format") && option.Get("pixel_format").IsString())
                streamConfig.pixelFormat = libcamera::PixelFormat::fromString(option.Get("pixel_format").As<Napi::String>().Utf8Value());
        }
        auto status = camera_config->validate();
        camera->configure(camera_config.get());
        state = Configured;
        camera->requestCompleted.connect(this, [this, &info](libcamera::Request *request) { this->requestComplete(info, request); });
        Napi::Array napi_stream_array = Napi::Array::New(info.Env(), camera_config->size());
        for (int i = 0; i < camera_config->size(); i++)
        {
            Napi::Object option = optionList.Get(i).As<Napi::Object>();
            libcamera::StreamConfiguration &streamConfig = camera_config->at(i);
            auto stream = streamConfig.stream();
            auto stream_obj =
                Stream::constructor->New({Napi::Number::New(info.Env(), i), Napi::Number::New(info.Env(), streamConfig.stride), Napi::String::New(info.Env(), streamConfig.colorSpace->toString()),
                                          Napi::Number::New(info.Env(), streamConfig.frameSize), Napi::Number::New(info.Env(), streamConfig.size.width),
                                          Napi::Number::New(info.Env(), streamConfig.size.height), Napi::String::New(info.Env(), streamConfig.pixelFormat.toString())});
            napi_stream_map[stream] = (Stream *)(&stream_obj);
            napi_stream_array[i] = stream_obj;
            streams.push_back(stream);
            stream_index_map[stream] = i;
            stream_config *_config = new stream_config();
            Stream::stream_config_map[i] = _config;
            // Napi::Object option = optionList.Get(i).As<Napi::Object>();
            if (option.Has("onImageData") && option.Get("onImageData").IsFunction())
            {
                auto onImageData = option.Get("onImageData").As<Napi::Function>();
                _config->callback_ref = Napi::Persistent(onImageData);
            }
            auto t1 = millis();
            for (unsigned int j = 0; j < streamConfig.bufferCount; j++)
            {
                if (requests.size() <= j)
                {
                    std::unique_ptr<libcamera::Request> req = camera->createRequest();
                    requests.push_back(std::move(req));
                }
                auto &request = requests.at(j);
                std::string name("rpicam-apps" + std::to_string(i));
                libcamera::UniqueFD fd = dma_heap_.alloc(name.c_str(), streamConfig.frameSize);
                std::vector<libcamera::FrameBuffer::Plane> plane(1);
                plane[0].fd = libcamera::SharedFD(std::move(fd));
                plane[0].offset = 0;
                plane[0].length = streamConfig.frameSize;
                auto buf = new libcamera::FrameBuffer(plane);
                request->addBuffer(stream, buf);
            }
        }
        return napi_stream_array;
    }

    Napi::Value start(const Napi::CallbackInfo &info)
    {
        if (state == Running)
        {
            return Napi::Number::New(info.Env(), -1);
        }
        if (worker)
        {
            delete worker;
        }
        auto now = millis();
        worker = new EchoWorker(info, camera.get(), &wait_deque, &napi_stream_map, &stream_index_map);
        worker->Queue();
        now = millis();
        int ret = camera->start();
        std::cout << "camera start cost: " << (millis() - now) << "ms" << std::endl;
        state = Running;
        requests_deque.clear();
        wait_deque.clear();
        // control_list.set(libcamera::controls::AfMode, libcamera::controls::AfModeContinuous);
        if (auto_queue_request)
        {
            std::cout << "start queue request at: " << millis() << std::endl;
            for (auto &req : requests)
            {
                if (req->status() == libcamera::Request::Status::RequestPending)
                {
                    req->controls().merge(control_list);
                    camera->queueRequest(req.get());
                }
                else
                {

                    req->reuse(libcamera::Request::ReuseBuffers);
                    req->controls().merge(control_list);
                    camera->queueRequest(req.get());
                }
            }
        }
        return Napi::Number::New(info.Env(), ret);
    }

    void requestComplete(const Napi::CallbackInfo &info, libcamera::Request *request)
    {
        struct dma_buf_sync dma_sync
        {
        };
        dma_sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
        for (auto const &buffer_map : request->buffers())
        {
            int ret = ::ioctl(buffer_map.second->planes()[0].fd.get(), DMA_BUF_IOCTL_SYNC, &dma_sync);
            if (ret)
                throw std::runtime_error("failed to sync dma buf on request complete");
        }
        wait_deque.push_back(request);
        worker->notify();
        // std::cout << "metadata size: " << request->metadata().size() << std::endl;

        requests_deque.push_back(request);

        if (requests_deque.size() >= requests.size())
        {
            auto front = requests_deque.front();
            front->reuse(libcamera::Request::ReuseBuffers);
            if (state == Running && auto_queue_request)
            {
                camera->queueRequest(front);
                requests_deque.pop_front();
            }
        }
    }

    Napi::Value config(const Napi::CallbackInfo &info)
    {
        auto option = info[0].As<Napi::Object>();
        if (option.Get("autoQueueRequest").IsBoolean())
        {
            this->auto_queue_request = option.Get("autoQueueRequest").As<Napi::Boolean>();
        }

        if (option.Get("maxFrameRate").IsNumber())
        {
            max_frame_rate = option.Get("maxFrameRate").As<Napi::Number>().FloatValue();
            int64_t frame_time = 1000000 / max_frame_rate; // in us
            control_list.set(libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({frame_time, frame_time}));
        }

        if (option.Get("crop").IsObject())
        {
            auto _crop = option.Get("crop").As<Napi::Object>();
            this->crop.roi_x = _crop.Get("roi_x").As<Napi::Number>().FloatValue();
            this->crop.roi_y = _crop.Get("roi_y").As<Napi::Number>().FloatValue();
            this->crop.roi_width = _crop.Get("roi_width").As<Napi::Number>().FloatValue();
            this->crop.roi_height = _crop.Get("roi_height").As<Napi::Number>().FloatValue();
            auto sensor_area = camera->properties().get(libcamera::properties::ScalerCropMaximum);
            int x = this->crop.roi_x * sensor_area->width;
            int y = this->crop.roi_y * sensor_area->height;
            int w = this->crop.roi_width * sensor_area->width;
            int h = this->crop.roi_height * sensor_area->height;
            libcamera::Rectangle crop(x, y, w, h);
            crop.translateBy(sensor_area->topLeft());
            control_list.set(libcamera::controls::ScalerCrop, crop);
        }
        return Napi::Number::New(info.Env(), 0);
    }

    Napi::Value queueRequest(const Napi::CallbackInfo &info)
    {
        if (state != Running)
        {
            return Napi::Number::New(info.Env(), -1);
        }
        if (requests_deque.size())
        {
            auto req = requests_deque.front();
            requests_deque.pop_front();
            camera->queueRequest(req);
        }
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
        state = Stopping;
        camera->stop();
        state = Configured;
        worker->stop();
        worker = nullptr;
        return Napi::Boolean::New(info.Env(), true);
    }

    Napi::Value release(const Napi::CallbackInfo &info)
    {
        camera->release();
        state = Available;

        return Napi::Boolean::New(info.Env(), true);
    }

    Napi::Value setMaxFrameRate(const Napi::CallbackInfo &info)
    {
        auto rate = info[0].As<Napi::Number>().FloatValue();
        // worker->setMaxFrameRate(rate);
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
                                              //   InstanceMethod<&Camera::setMaxFrameRate>("setMaxFrameRate", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                              InstanceMethod<&Camera::release>("release", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Camera", func);
        return exports;
    }
};

Napi::FunctionReference *Camera::constructor = new Napi::FunctionReference();
#endif
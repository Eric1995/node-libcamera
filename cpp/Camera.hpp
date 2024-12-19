#ifndef _CAMERA_H_
#define _CAMERA_H_ 1

#include <libcamera/libcamera.h>
#include <linux/dma-buf.h>
#include <napi.h>
#include <sys/ioctl.h>

#include "FrameWorker.hpp"
#include "utils/dma_heaps.hpp"
#include "utils/util.hpp"

static const std::map<int, std::string> cfa_map = {
    {libcamera::properties::draft::ColorFilterArrangementEnum::RGGB, "RGGB"}, {libcamera::properties::draft::ColorFilterArrangementEnum::GRBG, "GRBG"},
    {libcamera::properties::draft::ColorFilterArrangementEnum::GBRG, "GBRG"}, {libcamera::properties::draft::ColorFilterArrangementEnum::RGB, "RGB"},
    {libcamera::properties::draft::ColorFilterArrangementEnum::MONO, "MONO"},
};

static const std::map<libcamera::PixelFormat, unsigned int> bayer_formats = {
    {libcamera::formats::SRGGB10_CSI2P, 10}, {libcamera::formats::SGRBG10_CSI2P, 10}, {libcamera::formats::SBGGR10_CSI2P, 10}, {libcamera::formats::R10_CSI2P, 10},
    {libcamera::formats::SGBRG10_CSI2P, 10}, {libcamera::formats::SRGGB12_CSI2P, 12}, {libcamera::formats::SGRBG12_CSI2P, 12}, {libcamera::formats::SBGGR12_CSI2P, 12},
    {libcamera::formats::SGBRG12_CSI2P, 12}, {libcamera::formats::SRGGB16, 16},       {libcamera::formats::SGRBG16, 16},       {libcamera::formats::SBGGR16, 16},
    {libcamera::formats::SGBRG16, 16},
};

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
    FrameWorker *worker = nullptr;
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
        auto model = *camera->properties().get(libcamera::properties::Model);
        a.Set("model", model);
        auto config = camera->generateConfiguration({libcamera::StreamRole::Raw});
        const libcamera::StreamFormats &formats = config->at(0).formats();
        unsigned int bits = 0;
        for (const auto &pix : formats.pixelformats())
        {
            const auto &b = bayer_formats.find(pix);
            if (b != bayer_formats.end() && b->second > bits)
                bits = b->second;
        }
        if (bits)
        {
            a.Set("bit", bits);
        }
        auto cfa = camera->properties().get(libcamera::properties::draft::ColorFilterArrangement);
        if (cfa && cfa_map.count(*cfa))
        {
            a.Set("colorFilterArrangement", cfa_map.at(*cfa));
        }
        camera->acquire();

        // const libcamera::StreamFormats &formats = config->at(0).formats();
        Napi::Object modesObj = Napi::Object::New(info.Env());
        for (const auto &pix : formats.pixelformats())
        {
            Napi::Array modes = Napi::Array::New(info.Env());
            for (const auto &size : formats.sizes(pix))
            {
                Napi::Object mode = Napi::Object::New(info.Env());
                mode.Set("size", size.toString());
                mode.Set("pix", pix.toString());
                modes[modes.Length()] = mode;
                config->at(0).size = size;
                config->at(0).pixelFormat = pix;
                config->sensorConfig = libcamera::SensorConfiguration();
                config->sensorConfig->outputSize = size;
                std::string fmt = pix.toString();
                unsigned int mode_depth = fmt.find("8") != std::string::npos ? 8 : fmt.find("10") != std::string::npos ? 10 : fmt.find("12") != std::string::npos ? 12 : 16;
                config->sensorConfig->bitDepth = mode_depth;
                config->validate();
                camera->configure(config.get());
                auto fd_ctrl = camera->controls().find(&libcamera::controls::FrameDurationLimits);
                auto crop_ctrl = camera->properties().get(libcamera::properties::ScalerCropMaximum);
                double fps = fd_ctrl == camera->controls().end() ? NAN : (1e6 / fd_ctrl->second.min().get<int64_t>());
                mode.Set("crop", crop_ctrl->toString());
                mode.Set("framerate", fps);
            }
            modesObj.Set(pix.toString(), modes);
        }
        a.Set("modes", modesObj);
        camera->release();
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
                auto role = option.Get("role").As<Napi::Number>().Int32Value();
                if (role == 0)
                {
                    stream_roles.push_back(libcamera::StreamRole::Raw);
                }
                if (role == 2)
                {
                    stream_roles.push_back(libcamera::StreamRole::VideoRecording);
                }
                if (role == 1)
                {
                    stream_roles.push_back(libcamera::StreamRole::StillCapture);
                }
                if (role == 3)
                {
                    stream_roles.push_back(libcamera::StreamRole::Viewfinder);
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
        worker = new FrameWorker(info, camera.get(), &wait_deque, &napi_stream_map, &stream_index_map);
        worker->Queue();
        now = millis();
        int ret = camera->start();
        std::cout << "camera start cost: " << (millis() - now) << "ms" << std::endl;
        state = Running;
        requests_deque.clear();
        wait_deque.clear();
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
                front->controls().merge(control_list);
                camera->queueRequest(front);
                requests_deque.pop_front();
            }
        }
    }

    Napi::Value setControl(const Napi::CallbackInfo &info)
    {
        auto option = info[0].As<Napi::Object>();
        if (option.Get("crop").IsObject())
        {
            auto _crop = option.Get("crop").As<Napi::Object>();
            auto sensor_area = camera->properties().get(libcamera::properties::ScalerCropMaximum);
            float roi_x = _crop.Get("roi_x").As<Napi::Number>().FloatValue();
            float roi_y = _crop.Get("roi_y").As<Napi::Number>().FloatValue();
            float roi_width = _crop.Get("roi_width").As<Napi::Number>().FloatValue();
            float roi_height = _crop.Get("roi_height").As<Napi::Number>().FloatValue();
            int x = roi_x * sensor_area->width;
            int y = crop.roi_y * sensor_area->height;
            int w = crop.roi_width * sensor_area->width;
            int h = crop.roi_height * sensor_area->height;
            libcamera::Rectangle crop(x, y, w, h);
            crop.translateBy(sensor_area->topLeft());
            control_list.set(libcamera::controls::ScalerCrop, crop);
        }
        if (option.Get("ExposureTime").IsNumber())
            control_list.set(libcamera::controls::ExposureTime, option.Get("ExposureTime").As<Napi::Number>().Int64Value());
        if (option.Get("AnalogueGain").IsNumber())
            control_list.set(libcamera::controls::AnalogueGain, option.Get("AnalogueGain").As<Napi::Number>().FloatValue());
        if (option.Get("AeMeteringMode").IsNumber())
            control_list.set(libcamera::controls::AeMeteringMode, option.Get("AeMeteringMode").As<Napi::Number>().Int32Value());
        if (option.Get("AeExposureMode").IsNumber())
            control_list.set(libcamera::controls::AeExposureMode, option.Get("AeExposureMode").As<Napi::Number>().Int32Value());
        if (option.Get("ExposureValue").IsNumber())
            control_list.set(libcamera::controls::ExposureValue, option.Get("ExposureValue").As<Napi::Number>().FloatValue());
        if (option.Get("AwbMode").IsNumber())
            control_list.set(libcamera::controls::AwbMode, option.Get("AwbMode").As<Napi::Number>().Int32Value());
        if (option.Get("Brightness").IsNumber())
            control_list.set(libcamera::controls::Brightness, option.Get("Brightness").As<Napi::Number>().FloatValue());
        if (option.Get("Contrast").IsNumber())
            control_list.set(libcamera::controls::Contrast, option.Get("Contrast").As<Napi::Number>().FloatValue());
        if (option.Get("Saturation").IsNumber())
            control_list.set(libcamera::controls::Saturation, option.Get("Saturation").As<Napi::Number>().FloatValue());
        if (option.Get("Sharpness").IsNumber())
            control_list.set(libcamera::controls::Sharpness, option.Get("Sharpness").As<Napi::Number>().FloatValue());
        if (option.Get("AfRange").IsNumber())
            control_list.set(libcamera::controls::AfRange, option.Get("AfRange").As<Napi::Number>().Int32Value());
        if (option.Get("AfSpeed").IsNumber())
            control_list.set(libcamera::controls::AfSpeed, option.Get("AfSpeed").As<Napi::Number>().Int32Value());
        if (option.Get("LensPosition").IsNumber())
            control_list.set(libcamera::controls::LensPosition, option.Get("LensPosition").As<Napi::Number>().FloatValue());
        if (option.Get("AfMode").IsNumber())
            control_list.set(libcamera::controls::AfMode, option.Get("AfMode").As<Napi::Number>().Int32Value());
        if (option.Get("AfTrigger").IsNumber())
            control_list.set(libcamera::controls::AfTrigger, option.Get("AfTrigger").As<Napi::Number>().Int32Value());

        return info.Env().Undefined();
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
        if (requests_deque.size() < requests.size())
        {
            // std::cout << "queue request: " << requests_deque.size() << std::endl;
            auto req = requests[requests_deque.size()].get();
            if (req->status() == libcamera::Request::Status::RequestPending)
            {
                req->controls().merge(control_list);
                camera->queueRequest(req);
            }
            // requests_deque.pop_front();
        }
        else
        {
            auto front = requests_deque.front();
            front->reuse(libcamera::Request::ReuseBuffers);
            front->controls().merge(control_list);
            camera->queueRequest(front);
            requests_deque.pop_front();
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
        if (state == Available || state == Stopping)
        {
            return Napi::Boolean::New(info.Env(), true);
        }
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

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "Camera",
                                          {
                                              InstanceAccessor<&Camera::getId>("id", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::basicInfo>("getInfo", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::start>("start", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::queueRequest>("queueRequest", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::config>("config", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::stop>("stop", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::createStreams>("createStreams", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::getAvailableControls>("getAvailableControls", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::release>("release", static_cast<napi_property_attributes>(napi_enumerable)),
                                              InstanceMethod<&Camera::setControl>("setControl", static_cast<napi_property_attributes>(napi_enumerable)),
                                          });
        *constructor = Napi::Persistent(func);
        exports.Set("Camera", func);
        return exports;
    }
};

Napi::FunctionReference *Camera::constructor = new Napi::FunctionReference();
#endif
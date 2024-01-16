#include <deque>
#include <fcntl.h>
#include <iostream>
#include <libcamera/libcamera.h>
#include <linux/dma-buf.h>
#include <napi.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "./core/completed_request.hpp"
#include "dma_heaps.hpp"
#include "image.hpp"
#include "stream.hpp"
#include "util.hpp"

using namespace Napi;

using DataType = uint8_t *;

struct frame_buffer_t
{
    // libcamera::ControlList *metadata;
    libcamera::Request *request;
};

using WorkerType = frame_buffer_t *;

class CameraWorker : public AsyncProgressWorker<WorkerType>
{
  public:
    bool stopped = false;
    bool auto_queue_request = true;
    libcamera::ControlList control_list;
    std::shared_ptr<libcamera::Camera> camera;
    Napi::FunctionReference callback_ref;
    std::unique_ptr<libcamera::CameraConfiguration> camera_config = nullptr;
    std::deque<WorkerType> frame_data;
    std::deque<libcamera::Stream *> streams;
    std::map<libcamera::Stream *, stream_config *> stream_config_map;
    std::map<libcamera::Stream *, libcamera::ControlList> stream_controls;
    std::vector<std::unique_ptr<libcamera::Request>> requests;

    CameraWorker(const Napi::CallbackInfo &info, std::shared_ptr<libcamera::Camera> camera) : AsyncProgressWorker(info.Env()), camera(camera)
    {
    }

    ~CameraWorker()
    {
    }

    Napi::Array createStreams(const Napi::CallbackInfo &info)
    {
        Napi::Array optionList = info[0].As<Napi::Array>();
        std::vector<libcamera::StreamRole> stream_roles(optionList.Length());
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
                        stream_roles[i] = libcamera::StreamRole::Raw;
                    }
                    if (role == "VideoRecording")
                    {
                        stream_roles[i] = libcamera::StreamRole::VideoRecording;
                    }
                    if (role == "StillCapture")
                    {
                        stream_roles[i] = libcamera::StreamRole::StillCapture;
                    }
                    if (role == "Viewfinder")
                    {
                        stream_roles[i] = libcamera::StreamRole::Viewfinder;
                    }
                }
            }
        }
        camera->acquire();
        // camera->stop();
        auto config = camera->generateConfiguration(stream_roles);
        std::cout << "config generated" << std::endl;
        if(!camera_config) {
            camera_config = std::move(config);
        }else {
            camera_config.reset(config.get());
        }
        
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
        std::cout << "validate configuration" << std::endl;
        auto status = camera_config->validate();
        std::cout << "valute status: " << status << std::endl;
        camera->configure(camera_config.get());
        camera->requestCompleted.connect(this, [this, &info](libcamera::Request *request) { this->requestComplete(info, request); });
        streams.clear();
        stream_config_map.clear();
        requests.clear();
        std::cout << "create stream array" << std::endl;
        // auto _c = camera.get();
        Napi::Array napi_stream_array = Napi::Array::New(info.Env(), camera_config->size());
        for (int i = 0; i < camera_config->size(); i++)
        {
            Napi::Object option = optionList.Get(i).As<Napi::Object>();
            libcamera::StreamConfiguration &streamConfig = camera_config->at(i);
            auto stream = streamConfig.stream();
            auto stream_obj =
                Stream::constructor->New({Napi::Number::New(info.Env(), i), Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(camera.get())),
                                          Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(camera_config.get())), Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(&streamConfig)),
                                          Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(&stream_config_map))});
            napi_stream_array[i] = stream_obj;
            streams.push_back(stream);
            stream_config *_config = new stream_config();
            stream_config_map[stream] = _config;
            std::cout << "ready to alloc dma heap buffer" << std::endl;

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
                std::cout << "dma heap alloced" << std::endl;
                std::vector<libcamera::FrameBuffer::Plane> plane(1);
                plane[0].fd = libcamera::SharedFD(std::move(fd));
                plane[0].offset = 0;
                plane[0].length = streamConfig.frameSize;
                auto buf = new libcamera::FrameBuffer(plane);
                request->addBuffer(stream, buf);
            }
        }
        std::cout << "create streams finished" << std::endl;
        return napi_stream_array;
    }

    int start()
    {
        stopped = false;
        int ret = camera->start();
        // control_list.set(libcamera::controls::AfMode, libcamera::controls::AfModeContinuous);
        if (auto_queue_request)
        {
            for (auto &req : requests)
            {
                req->controls().merge(control_list);
                camera->queueRequest(req.get());
            }
        }
        return ret;
    }

    void setMaxFrameRate(float rate)
    {
        int64_t frame_time = 1000000 / rate; // in us
        control_list.set(libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({frame_time, frame_time}));
    }

    void setCrop(float roi_x, float roi_y, float roi_width, float roi_height)
    {
        auto sensor_area = camera->properties().get(libcamera::properties::ScalerCropMaximum);
        int x = roi_x * sensor_area->width;
        int y = roi_y * sensor_area->height;
        int w = roi_width * sensor_area->width;
        int h = roi_height * sensor_area->height;
        libcamera::Rectangle crop(x, y, w, h);
        crop.translateBy(sensor_area->topLeft());
        control_list.set(libcamera::controls::ScalerCrop, crop);
    }

    void queueRequest()
    {
        if (stopped || auto_queue_request)
            return;
        for (auto &req : requests)
        {
            if (req->status() == libcamera::Request::Status::RequestPending)
            {
                req->controls().merge(control_list);
                camera->queueRequest(req.get());
                break;
            }
        }
    }

    std::vector<libcamera::StreamConfiguration> getStreamConfig()
    {
        std::vector<libcamera::StreamConfiguration> config_vec;
        for (auto stream : streams)
        {
            config_vec.push_back(stream->configuration());
        }
        return config_vec;
    }

    void requestComplete(const Napi::CallbackInfo &info, libcamera::Request *request)
    {
        if (stopped)
        {
            return;
        }
        // request status is COMPLETE
        frame_buffer_t *frame_buffer_data = new frame_buffer_t();
        frame_buffer_data->request = request;
        // frame_buffer_data->metadata = new libcamera::ControlList(request->metadata());
        frame_data.push_back(frame_buffer_data);
    }

    static libcamera::PixelFormat get_format(const char *format)
    {
        auto f = libcamera::PixelFormat::fromString(std::string(format));
        return f;
    }

    void StopCamera()
    {
        stopped = true;
        camera->stop();
        camera->release();
        // camera.reset();
    }

    void Execute(const ExecutionProgress &progress)
    {
        while (true)
        {
            if (frame_data.size() == 0)
            {
                if (stopped)
                {
                    break;
                }
                usleep(5 * 1000);
                continue;
            }
            else
            {
                while (frame_data.size())
                {
                    frame_buffer_t *t = frame_data.front();
                    progress.Send(&t, sizeof(frame_buffer_t));
                    frame_data.pop_front();
                }
            }
        }
    }

    void OnError(const Error &e)
    {
        HandleScope scope(Env());
        // if (!callback_ref.IsEmpty())
        //     callback_ref.Call({String::New(Env(), e.Message())});
    }

    void OnOK()
    {
        HandleScope scope(Env());
        // if (!callback_ref.IsEmpty())
        //     callback_ref.Call({Env().Null(), String::New(Env(), "Ok")});
    }

    void OnProgress(const WorkerType *data, size_t t)
    {
        HandleScope scope(Env());
        if (*data && (*data)->request)
        {
            auto req_addr = Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>((*data)->request));
            libcamera::Stream *stream;
            Napi::Array image_list = Napi::Array::New(Env());
            int n = 0;
            for (auto &pair : (*data)->request->buffers())
            {
                stream = (libcamera::Stream *)pair.first;
                Napi::Object image = Image::constructor->New({Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(camera.get())), Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(stream)),
                                                              req_addr, Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(stream_config_map[stream]))});
                image_list[n] = image;
                auto config = stream_config_map[stream];
                auto &callback = config->callback_ref;
                if (!callback.IsEmpty())
                {
                    callback.Call({Env().Null(), Env().Null(), image});
                }
                n++;
            }
            if (!callback_ref.IsEmpty())
                callback_ref.Call({Env().Null(), Env().Null(), image_list});
            (*data)->request->reuse(libcamera::Request::ReuseBuffers);
            if (auto_queue_request && !stopped)
            {
                (*data)->request->controls().merge(control_list);
                camera->queueRequest((*data)->request);
            }
        }
        delete (*data);
    }

  private:
    DmaHeap dma_heap_;
};
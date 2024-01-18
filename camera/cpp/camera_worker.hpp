#include <condition_variable>
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

using WorkerType = void *;

class CameraWorker : public AsyncProgressQueueWorker<WorkerType>
{
  public:
    enum Status
    {
        Available,
        Acquired,
        Configured,
        Stopping,
        Running
    };
    Status state = Available;
    bool stopped = false;
    bool auto_queue_request = true;
    std::mutex mtx; // 互斥量，保护产品缓冲区
    std::condition_variable frame_available;
    libcamera::ControlList control_list;
    std::shared_ptr<libcamera::Camera> camera;
    Napi::FunctionReference callback_ref;
    std::unique_ptr<libcamera::CameraConfiguration> camera_config = nullptr;
    std::deque<libcamera::Stream *> streams;
    std::map<libcamera::Stream *, stream_config *> stream_config_map;
    std::vector<std::unique_ptr<libcamera::Request>> requests;
    std::deque<libcamera::Request *> requests_deque;
    std::deque<libcamera::Request *> wait_deque;

    CameraWorker(const Napi::CallbackInfo &info, std::shared_ptr<libcamera::Camera> camera) : AsyncProgressQueueWorker(info.Env()), camera(camera)
    {
        dma_heap_ = new DmaHeap();
    }

    ~CameraWorker()
    {
        std::cout << "camera worker destroyed" << std::endl;
        clean();
        delete dma_heap_;
    }

    void clean()
    {
        streams.clear();
        stream_config_map.clear();
        for (auto &a : stream_config_map)
        {
            delete a.second;
        }
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

    Napi::Array createStreams(const Napi::CallbackInfo &info)
    {
        clean();
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
                Stream::constructor->New({Napi::Number::New(info.Env(), i), Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(camera.get())),
                                          Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(camera_config.get())), Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(&streamConfig)),
                                          Napi::BigInt::New(info.Env(), reinterpret_cast<uint64_t>(&stream_config_map))});
            napi_stream_array[i] = stream_obj;
            streams.push_back(stream);
            stream_config *_config = new stream_config();
            stream_config_map[stream] = _config;

            for (unsigned int j = 0; j < streamConfig.bufferCount; j++)
            {
                if (requests.size() <= j)
                {
                    std::unique_ptr<libcamera::Request> req = camera->createRequest();
                    requests.push_back(std::move(req));
                }
                auto &request = requests.at(j);
                std::string name("rpicam-apps" + std::to_string(i));
                libcamera::UniqueFD fd = dma_heap_->alloc(name.c_str(), streamConfig.frameSize);
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

    int start()
    {
        stopped = false;
        int ret = camera->start();
        state = Running;
        requests_deque.clear();
        wait_deque.clear();
        // control_list.set(libcamera::controls::AfMode, libcamera::controls::AfModeContinuous);
        if (auto_queue_request)
        {
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
        // std::cout << "request complete, requests_deque size: " << requests_deque.size() << std::endl;
        // if (stopped)
        // {
        //     frame_available.notify_all();
        //     std::cout << "notify all for stop" << std::endl;
        //     return;
        // }
        wait_deque.push_back(request);
        requests_deque.push_back(request);
        if (requests_deque.size() >= requests.size())
        {
            auto front = requests_deque.front();
            requests_deque.pop_front();
            front->reuse(libcamera::Request::ReuseBuffers);
            if (state == Running)
            {
                camera->queueRequest(front);
            }
        }
        // std::cout << "notify all for execute" << std::endl;
        frame_available.notify_all();
    }

    static libcamera::PixelFormat get_format(const char *format)
    {
        auto f = libcamera::PixelFormat::fromString(std::string(format));
        return f;
    }

    void StopCamera()
    {
        stopped = true;
        state = Stopping;
        camera->stop();
        state = Configured;
    }

    void Release()
    {
        if (state == Running)
        {
            StopCamera();
        }
        camera->release();
        state = Available;
        frame_available.notify_all();
    }

    void Execute(const ExecutionProgress &progress)
    {
        while (true)
        {
            std::unique_lock<std::mutex> lck(mtx);
            frame_available.wait(lck);
            progress.Signal();
            if (state == Available)
            {
                std::cout << "break execute" << std::endl;
                break;
            }
        }
        std::cout << "exit execute" << std::endl;
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
        while (wait_deque.size())
        {
            auto request = wait_deque.front();
            std::cout << "metadata size: " << request->metadata().size() << std::endl;
            wait_deque.pop_front();
            auto req_addr = Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(request));
            for (auto &pair : request->buffers())
            {
                libcamera::Stream *stream = (libcamera::Stream *)pair.first;
                Napi::Object image = Image::constructor->New({Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(camera.get())), Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(stream)),
                                                              req_addr, Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(stream_config_map[stream]))});
                auto config = stream_config_map[stream];
                auto &callback = config->callback_ref;
                if (!callback.IsEmpty())
                {
                    callback.Call({Env().Null(), Env().Null(), image});
                }
            }
        }
    }

  private:
    DmaHeap *dma_heap_;
};
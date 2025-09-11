#include "Camera.h"

Napi::FunctionReference *Stream::constructor = new Napi::FunctionReference();
Napi::FunctionReference *Image::constructor = new Napi::FunctionReference();
Napi::FunctionReference *Camera::constructor = new Napi::FunctionReference();

Camera::Camera(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Camera>(info)
{
    this->cm = *info[0].As<Napi::External<std::shared_ptr<libcamera::CameraManager>>>().Data();
    this->camera = *info[1].As<Napi::External<std::shared_ptr<libcamera::Camera>>>().Data();
}

Camera::~Camera()
{
    if (state != Available)
    {
        camera->stop();
        camera->release();
    }
    clean();
}

void Camera::clean()
{
    streams.clear();
    // 释放对 Stream 对象的持久化引用，允许它们被 GC
    for (auto &pair : napi_stream_map)
    {
        pair.second.Reset();
    }
    napi_stream_map.clear();
    stream_index_map.clear();
    requests.clear();
    requests_deque.clear();
    wait_deque.clear();
    camera->requestCompleted.disconnect();
}

Napi::Value Camera::createStreams(const Napi::CallbackInfo &info)
{
    clean();
    Napi::HandleScope scope(info.Env());
    stream_roles.clear();
    Napi::Array optionList = info[0].As<Napi::Array>();
    for (int i = 0; i < optionList.Length(); i++)
    {
        Napi::Object option = optionList.Get(i).As<Napi::Object>();
        if (option.Get("role").IsNumber())
        {
            auto role = option.Get("role").As<Napi::Number>().Int32Value();
            static const std::map<int, libcamera::StreamRole> role_map = {
                {0, libcamera::StreamRole::Raw}, {1, libcamera::StreamRole::StillCapture}, {2, libcamera::StreamRole::VideoRecording}, {3, libcamera::StreamRole::Viewfinder}};
            if (auto it = role_map.find(role); it != role_map.end())
                stream_roles.push_back(it->second);
        }
    }
    if (state == Available)
    {
        camera->acquire();
        state = Acquired;
    }
    camera_config = camera->generateConfiguration(stream_roles);
    camera_config->orientation = camera_config->orientation * (vflip ? libcamera::Transform::VFlip : libcamera::Transform::Identity) *
                                 (hflip ? libcamera::Transform::HFlip : libcamera::Transform::Identity) * libcamera::transformFromRotation(this->rotation);

    if (sensorMode.width > 0 && sensorMode.height > 0 && sensorMode.bitDepth > 0)
    {
        camera_config->sensorConfig = libcamera::SensorConfiguration();
        camera_config->sensorConfig->outputSize = libcamera::Size(sensorMode.width, sensorMode.height);
        camera_config->sensorConfig->bitDepth = sensorMode.bitDepth;
    }
    for (int i = 0; i < optionList.Length(); i++)
    {
        Napi::Object option = optionList.Get(i).As<Napi::Object>();
        libcamera::StreamConfiguration &streamConfig = camera_config->at(i);
        if (option.Get("width").IsNumber())
            streamConfig.size.width = option.Get("width").As<Napi::Number>().Uint32Value();
        if (option.Get("height").IsNumber())
            streamConfig.size.height = option.Get("height").As<Napi::Number>().Uint32Value();
        if (option.Get("pixel_format").IsString())
            streamConfig.pixelFormat = libcamera::PixelFormat::fromString(option.Get("pixel_format").As<Napi::String>().Utf8Value());
        if (option.Has("bufferCount") && option.Get("bufferCount").IsNumber())
            streamConfig.bufferCount = option.Get("bufferCount").As<Napi::Number>().Uint32Value();
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
        Napi::Value onImageData = info.Env().Undefined();
        if (option.Has("onImageData") && option.Get("onImageData").IsFunction())
        {
            Napi::Function jsFunc = option.Get("onImageData").As<Napi::Function>();
            onImageData = option.Get("onImageData");
        }
        auto external_config = Napi::External<libcamera::StreamConfiguration>::New(info.Env(), &streamConfig);
        auto stream_obj = Stream::constructor->New({Napi::Number::New(info.Env(), i), external_config, onImageData});
        auto stream = streamConfig.stream();
        napi_stream_map[stream] = Napi::Persistent(stream_obj);
        napi_stream_map[stream].SuppressDestruct(); // 因为它由 map 管理，在 clean() 中手动释放
        napi_stream_array[i] = stream_obj;
        streams.push_back(stream);
        stream_index_map[stream] = i;

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

Napi::Value Camera::start(const Napi::CallbackInfo &info)
{
    Napi::HandleScope scope(info.Env());
    if (state == Running)
        return Napi::Number::New(info.Env(), -1);
    worker = std::make_unique<FrameWorker>(info, &wait_deque, &napi_stream_map);
    worker->Queue();
    int ret = camera->start();
    state = Running;
    requests_deque.clear();
    wait_deque.clear();
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
    return Napi::Number::New(info.Env(), ret);
}

void Camera::requestComplete(const Napi::CallbackInfo &info, libcamera::Request *request)
{
    struct dma_buf_sync dma_sync{};
    dma_sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
    for (auto const &buffer_map : request->buffers())
    {
        int ret = ::ioctl(buffer_map.second->planes()[0].fd.get(), DMA_BUF_IOCTL_SYNC, &dma_sync);
        if (ret)
            throw std::runtime_error("failed to sync dma buf on request complete");
    }
    wait_deque.push_back(request);
    worker->notify();
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

// 用户主动发送帧请求
Napi::Value Camera::sendRequest(const Napi::CallbackInfo &info)
{
    if (state != Running)
        return Napi::Number::New(info.Env(), -1);
    if (requests_deque.size() < requests.size())
    {
        auto req = requests[requests_deque.size()].get();
        if (req->status() == libcamera::Request::Status::RequestPending)
        {
            req->controls().merge(control_list);
            camera->queueRequest(req);
        }
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

Napi::Value Camera::stop(const Napi::CallbackInfo &info)
{
    if (state == Available || state == Stopping)
        return Napi::Boolean::New(info.Env(), true);
    state = Stopping;
    camera->stop();
    state = Configured;
    worker->stop();
    return Napi::Boolean::New(info.Env(), true);
}

Napi::Value Camera::release(const Napi::CallbackInfo &info)
{
    camera->release();
    state = Available;
    return Napi::Boolean::New(info.Env(), true);
}

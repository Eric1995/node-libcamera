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
    for (auto const &[key, val] : Stream::stream_config_map)
    {
        delete val;
    }
    Stream::stream_config_map.clear();
    streams.clear();
    stream_index_map.clear();
    requests.clear();
    requests_deque.clear();
    wait_deque.clear();
    camera->requestCompleted.disconnect();
}

Napi::Value Camera::getId(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, camera->id());
};

Napi::Value Camera::basicInfo(const Napi::CallbackInfo &info)
{
    Napi::HandleScope scope(info.Env());
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
        a.Set("bit", bits);
    auto cfa = camera->properties().get(libcamera::properties::draft::ColorFilterArrangement);
    if (cfa && cfa_map.count(*cfa))
        a.Set("colorFilterArrangement", cfa_map.at(*cfa));
    camera->acquire();

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
        auto stream = streamConfig.stream();
        auto stream_obj =
            Stream::constructor->New({Napi::Number::New(info.Env(), i), Napi::Number::New(info.Env(), streamConfig.stride), Napi::String::New(info.Env(), streamConfig.colorSpace->toString()),
                                      Napi::Number::New(info.Env(), streamConfig.frameSize), Napi::Number::New(info.Env(), streamConfig.size.width),
                                      Napi::Number::New(info.Env(), streamConfig.size.height), Napi::String::New(info.Env(), streamConfig.pixelFormat.toString())});
        napi_stream_map[stream] = Stream::Unwrap(stream_obj);
        napi_stream_array[i] = stream_obj;
        streams.push_back(stream);
        stream_index_map[stream] = i;
        stream_config *_config = new stream_config();
        Stream::stream_config_map[i] = _config;

        if (option.Has("onImageData") && option.Get("onImageData").IsFunction())
            _config->callback_ref = Napi::Persistent(option.Get("onImageData").As<Napi::Function>());
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
    worker = std::make_unique<FrameWorker>(info, camera.get(), &wait_deque, &napi_stream_map, &stream_index_map);
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

void Camera::setCrop(Napi::Object _crop)
{
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

Napi::Value Camera::setControl(const Napi::CallbackInfo &info)
{
    Napi::HandleScope scope(info.Env());
    auto option = info[0].As<Napi::Object>();
    if (option.Get("crop").IsObject())
        setCrop(option.Get("crop").As<Napi::Object>());
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

Napi::Value Camera::config(const Napi::CallbackInfo &info)
{
    Napi::HandleScope scope(info.Env());
    auto option = info[0].As<Napi::Object>();
    if (option.Get("autoQueueRequest").IsBoolean())
        this->auto_queue_request = option.Get("autoQueueRequest").As<Napi::Boolean>();
    if (option.Get("maxFrameRate").IsNumber())
    {
        max_frame_rate = option.Get("maxFrameRate").As<Napi::Number>().FloatValue();
        int64_t frame_time = 1000000 / max_frame_rate; // in us
        control_list.set(libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({frame_time, frame_time}));
    }
    if (option.Get("crop").IsObject())
        setCrop(option.Get("crop").As<Napi::Object>());
    if (option.Get("vflip").IsBoolean())
        this->vflip = option.Get("vflip").As<Napi::Boolean>();
    if (option.Get("hflip").IsBoolean())
        this->hflip = option.Get("hflip").As<Napi::Boolean>();
    if (option.Get("rotation").IsNumber())
        this->rotation = option.Get("rotation").As<Napi::Number>().Int32Value();
    if (option.Get("sensorMode").IsObject())
    {
        auto _sensorMode = option.Get("sensorMode").As<Napi::Object>();
        if (_sensorMode.Get("width").IsNumber())
            sensorMode.width = _sensorMode.Get("width").As<Napi::Number>().Uint32Value();
        if (_sensorMode.Get("height").IsNumber())
            sensorMode.height = _sensorMode.Get("height").As<Napi::Number>().Uint32Value();
        if (_sensorMode.Get("bitDepth").IsNumber())
            sensorMode.bitDepth = _sensorMode.Get("bitDepth").As<Napi::Number>().Uint32Value();
        if (_sensorMode.Get("packed").IsBoolean())
            sensorMode.packed = _sensorMode.Get("packed").As<Napi::Boolean>();
    }
    return Napi::Number::New(info.Env(), 0);
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

Napi::Value Camera::getAvailableControls(const Napi::CallbackInfo &info)
{
    Napi::HandleScope scope(info.Env());
    auto control_map = camera->controls();
    Napi::Object obj = Napi::Object::New(info.Env());
    for (auto const &[id, info] : control_map)
        obj.Set(id->name(), info.toString());
    return obj;
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

Napi::Object Camera::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env, "Camera",
                                      {
                                          InstanceAccessor<&Camera::getId>("id", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceMethod<&Camera::basicInfo>("getInfo", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceMethod<&Camera::start>("start", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceMethod<&Camera::sendRequest>("sendRequest", static_cast<napi_property_attributes>(napi_enumerable)),
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
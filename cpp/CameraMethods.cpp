#include "Camera.h"

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

Napi::Value Camera::getAvailableControls(const Napi::CallbackInfo &info)
{
    Napi::HandleScope scope(info.Env());
    auto control_map = camera->controls();
    Napi::Object obj = Napi::Object::New(info.Env());
    for (auto const &[id, info] : control_map)
        obj.Set(id->name(), info.toString());
    return obj;
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
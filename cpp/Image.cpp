#include "Image.h"

SaveWorker::SaveWorker(Function &callback, uint8_t _type, std::string _filename, uint32_t _frame_size, uint8_t _quality, uint8_t *_plane_data, const libcamera::ControlList &_metadata,
                       libcamera::Stream *_stream)
    : AsyncWorker(callback), type(_type), filename(_filename), frame_size(_frame_size), quality(_quality), plane_data(_plane_data), metadata(_metadata), stream(_stream)
{
}
SaveWorker::~SaveWorker()
{
}
void SaveWorker::Execute()
{
    auto file_name = filename;
    auto data = libcamera::Span<uint8_t>(plane_data, frame_size);
    std::vector vec{data};
    StreamInfo stream_info;
    stream_info.width = stream->configuration().size.width;
    stream_info.height = stream->configuration().size.height;
    stream_info.stride = stream->configuration().stride;
    stream_info.pixel_format = stream->configuration().pixelFormat;
    stream_info.colour_space = stream->configuration().colorSpace;
    StillOptions *options = new StillOptions();
    if (type == 1)
    {
        dng_save(vec, stream_info, metadata, file_name, "picam", options);
    }
    if (type == 2)
    {
        options->quality = quality;
        jpeg_save(vec, stream_info, metadata, file_name, "picam", options);
    }
    if (type == 3)
    {
        yuv_save(vec, stream_info, file_name, nullptr);
    }
    if (type == 4)
    {
        qoi_save(vec, stream_info, file_name, nullptr);
    }
    delete[] plane_data;
    delete options;
}
void SaveWorker::OnOK()
{
    HandleScope scope(Env());
    Callback().Call({Env().Null()});
}

Image::Image(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Image>(info)
{
    bool lossless = true;
    stream = info[0].As<Napi::External<libcamera::Stream>>().Data();
    request = info[1].As<Napi::External<libcamera::Request>>().Data();
    metadata = new libcamera::ControlList(request->metadata());
    // request status is COMPLETE
    auto buffers = request->buffers();
    buffer = buffers.at(stream);
    frame_size = stream->configuration().frameSize;
    auto plane = buffer->planes()[0];
    fd = plane.fd.get();
}

Image::~Image()
{
    delete metadata;
    if (memory != nullptr)
    {
        munmap(memory, frame_size);
        memory = nullptr;
    }
    delete[] data;
}

Napi::Value Image::getFd(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), fd);
}

Napi::Value Image::getFrameSize(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), frame_size);
}

Napi::Value Image::getData(const Napi::CallbackInfo &info)
{
    if (data != nullptr)
    {
        Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), data, stream->configuration().frameSize, [](Napi::Env env, void *arg) {});
        return buf;
    }

    if (memory == nullptr)
    {
        memory = mmap(NULL, frame_size, PROT_READ, MAP_SHARED, fd, 0);
        if (memory == MAP_FAILED)
        {
            Napi::Error::New(info.Env(), "mmap failed").ThrowAsJavaScriptException();
            return info.Env().Undefined();
        }
    }
    data = new uint8_t[frame_size];
    memcpy(data, memory, frame_size);
    Napi::ArrayBuffer buf = Napi::ArrayBuffer::New(info.Env(), data, stream->configuration().frameSize, [](Napi::Env env, void *arg) {});
    return buf;
}

Napi::Value Image::getColorSpace(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), stream->configuration().colorSpace->toString());
}

Napi::Value Image::getStride(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), stream->configuration().stride);
}

Napi::Value Image::getPixelFormat(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), stream->configuration().pixelFormat.toString());
}

Napi::Value Image::getPixelFormatFourcc(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), stream->configuration().pixelFormat.fourcc());
}

Napi::Value convertControlValue(Napi::Env env, const libcamera::ControlValue &value)
{
    if (value.isArray())
    {
        Napi::Array arr = Napi::Array::New(env);
        auto values = value.data();
        auto numElements = value.numElements();
        for (uint32_t i = 0; i < numElements; i++)
        {
            auto step = values.size() / numElements;
            auto ele = values.subspan(i * step, step);
            uint64_t result = 0;
            std::memcpy(&result, ele.data(), step);
            arr.Set(i, Napi::Number::New(env, result));
        }
        return arr;
    }
    switch (value.type())
    {
    case libcamera::ControlType::ControlTypeNone:
        return env.Null();
    case libcamera::ControlType::ControlTypeBool:
        return Napi::Boolean::New(env, value.get<bool>());
    case libcamera::ControlType::ControlTypeByte:
        return Napi::Number::New(env, value.get<unsigned char>());
    case libcamera::ControlType::ControlTypeUnsigned16:
        return Napi::Number::New(env, value.get<uint16_t>());
    case libcamera::ControlType::ControlTypeUnsigned32:
        return Napi::Number::New(env, value.get<uint32_t>());
    case libcamera::ControlType::ControlTypeInteger32:
        return Napi::Number::New(env, value.get<int32_t>());
    case libcamera::ControlType::ControlTypeInteger64:
        // JavaScript numbers are doubles, which can safely represent integers up to 2^53.
        // int64_t might exceed this, but for most camera metadata it's fine.
        return Napi::Number::New(env, static_cast<double>(value.get<int64_t>()));
    case libcamera::ControlType::ControlTypeFloat:
        return Napi::Number::New(env, value.get<float>());
    case libcamera::ControlType::ControlTypeString:
        return Napi::String::New(env, value.get<std::string>());
    case libcamera::ControlType::ControlTypeRectangle: {
        libcamera::Rectangle r = value.get<libcamera::Rectangle>();
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("x", r.x);
        obj.Set("y", r.y);
        obj.Set("width", r.width);
        obj.Set("height", r.height);
        return obj;
    }
    case libcamera::ControlType::ControlTypeSize: {
        libcamera::Size s = value.get<libcamera::Size>();
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("width", s.width);
        obj.Set("height", s.height);
        return obj;
    }
    case libcamera::ControlType::ControlTypePoint: {
        libcamera::Point p = value.get<libcamera::Point>();
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("x", p.x);
        obj.Set("y", p.y);
        return obj;
    }
    default:
        // For complex types like arrays, return their string representation.
        return Napi::String::New(env, value.toString());
    }
}

Napi::Value Image::getMetadata(const Napi::CallbackInfo &info)
{
    Napi::Object metadata_obj = Napi::Object::New(info.Env());
    const auto &id_map = *metadata->idMap();
    for (auto const &[id, value] : *metadata)
        metadata_obj.Set(id_map.at(id)->name(), convertControlValue(info.Env(), value));
    return metadata_obj;
}

Napi::Value Image::save(const Napi::CallbackInfo &info)
{
    auto option = info[0].As<Napi::Object>();
    auto file_name = option.Get("file").As<Napi::String>().ToString();
    auto type = option.Get("type").As<Napi::Number>().Uint32Value();
    uint8_t quality = 90;
    if (option.Has("quality") && option.Get("quality").IsNumber())
        quality = option.Get("quality").As<Napi::Number>().Uint32Value();
    Function cb = info[1].As<Function>();
    auto plane = buffer->planes()[0];
    void *map_mem = mmap(NULL, frame_size, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
    uint8_t *copy_mem = new uint8_t[frame_size];
    memcpy(copy_mem, map_mem, frame_size);
    if (map_mem != MAP_FAILED)
        munmap(map_mem, frame_size);
    auto wk = new SaveWorker(cb, type, file_name, frame_size, quality, copy_mem, *metadata, stream);
    wk->Queue();
    return info.Env().Undefined();
}

Napi::Object Image::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env, "Image",
                                      {
                                          InstanceAccessor<&Image::getFd>("fd", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceAccessor<&Image::getFrameSize>("frameSize", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceMethod<&Image::getData>("getData", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceAccessor<&Image::getColorSpace>("colorSpace", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceAccessor<&Image::getStride>("stride", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceAccessor<&Image::getPixelFormat>("pixelFormat", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceAccessor<&Image::getPixelFormatFourcc>("pixelFormatFourcc", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceMethod<&Image::save>("save", static_cast<napi_property_attributes>(napi_enumerable)),
                                          InstanceMethod<&Image::getMetadata>("getMetadata", static_cast<napi_property_attributes>(napi_enumerable)),
                                      });
    constructor = Napi::Persistent(func);
    exports.Set("Image", func);
    return exports;
}

Napi::FunctionReference Image::constructor;
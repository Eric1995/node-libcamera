#include "Image.h"

SaveWorker::SaveWorker(Function &callback, uint8_t _type, std::string _filename, uint32_t _frame_size, uint8_t _quality, uint8_t *_plane_data, libcamera::ControlList *_metadata,
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
        dng_save(vec, stream_info, *metadata, file_name, "picam", options);
    }
    if (type == 2)
    {
        options->quality = quality;
        jpeg_save(vec, stream_info, *metadata, file_name, "picam", options);
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
    // printf("Image object destroyed for request %p\n", request);
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
    auto wk = new SaveWorker(cb, type, file_name, frame_size, quality, copy_mem, metadata, stream);
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
                                      });
    *constructor = Napi::Persistent(func);
    exports.Set("Image", func);
    return exports;
}
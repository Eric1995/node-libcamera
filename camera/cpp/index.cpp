#include <libcamera/libcamera.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <linux/dma-buf.h>
#include <map>
#include <memory>
#include <napi.h>
#include <thread>
#include <unistd.h>

#include "camera.hpp"
#include "camera_manager.hpp"
// #include "h264_encoder.hpp"
// #include "./encoder/mjpeg_encoder.hpp"
#include "stream.hpp"
#include "image.hpp"

// Initialize native add-on
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    CameraManager::Init(env, exports);
    Camera::Init(env, exports);
    // H264Encoder::Init(env, exports);
    // MJPEGEncoder::Init(env, exports);
    Stream::Init(env, exports);
    Image::Init(env, exports);

    return exports;
}

NODE_API_MODULE(addon, Init)
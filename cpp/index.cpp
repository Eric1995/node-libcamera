#include <napi.h>

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Image.hpp"
#include "Stream.hpp"

// Initialize native add-on
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    CameraManager::Init(env, exports);
    Camera::Init(env, exports);
    Stream::Init(env, exports);
    Image::Init(env, exports);

    return exports;
}

NODE_API_MODULE(addon, Init)
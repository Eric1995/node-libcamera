#pragma once

#include <libcamera/libcamera.h>
#include <linux/dma-buf.h>
#include <napi.h>
#include <sys/ioctl.h>

#include "FrameWorker.h"
#include "utils/dma_heaps.h"
#include "utils/util.h"

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

struct crop_t
{
    float roi_x = 0;
    float roi_y = 0;
    float roi_width = 1;
    float roi_height = 1;
};

struct sensor_mode
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bitDepth = 0;
    bool packed = true;
};

enum Status
{
    Available,
    Acquired,
    Configured,
    Stopping,
    Running
};

class Camera : public Napi::ObjectWrap<Camera>
{
  private:
    FrameWorker *frame_worker = nullptr; // 用于调用 stop() 的原始指针

    Status state = Available;
    uint32_t num = 0;
    std::shared_ptr<libcamera::CameraManager> cm;
    std::shared_ptr<libcamera::Camera> camera;
    bool queued = false;
    bool released = false;
    sensor_mode sensorMode;

    /** 是否自动发送request请求*/
    bool auto_queue_request = true;
    float max_frame_rate = 30.0;
    crop_t crop;
    bool vflip = false;
    bool hflip = false;
    int rotation = 0;

    std::vector<libcamera::StreamRole> stream_roles;
    std::unique_ptr<libcamera::CameraConfiguration> camera_config = nullptr;
    libcamera::ControlList control_list;
    std::deque<libcamera::Stream *> streams;
    std::vector<std::unique_ptr<libcamera::Request>> requests;
    std::deque<libcamera::Request *> requests_deque;
    std::deque<libcamera::Request *> wait_deque;
    // 保存对Stream的引用,防止被JS垃圾回收
    std::map<libcamera::Stream *, Napi::ObjectReference> napi_stream_map;
    std::map<libcamera::Stream *, unsigned int> stream_index_map;

    DmaHeap dma_heap_;

    void setCrop(Napi::Object crop);

  public:
    static Napi::FunctionReference constructor;

    Camera(const Napi::CallbackInfo &info);

    ~Camera();

    void clean();

    Napi::Value getId(const Napi::CallbackInfo &info);
    Napi::Value basicInfo(const Napi::CallbackInfo &info);

    Napi::Value createStreams(const Napi::CallbackInfo &info);

    Napi::Value start(const Napi::CallbackInfo &info);

    void requestComplete(const Napi::CallbackInfo &info, libcamera::Request *request);

    Napi::Value setControls(const Napi::CallbackInfo &info);

    Napi::Value removeControl(const Napi::CallbackInfo &info);

    Napi::Value config(const Napi::CallbackInfo &info);

    // 用户主动发送帧请求
    Napi::Value sendRequest(const Napi::CallbackInfo &info);

    Napi::Value getAvailableControls(const Napi::CallbackInfo &info);

    Napi::Value stop(const Napi::CallbackInfo &info);

    Napi::Value release(const Napi::CallbackInfo &info);

    Napi::Value resetControls(const Napi::CallbackInfo &info);

    static Napi::Object Init(Napi::Env env, Napi::Object exports);
};
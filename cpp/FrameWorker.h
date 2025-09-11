#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <libcamera/libcamera.h>
#include <napi.h>
#include <thread>

#include "Image.h"
#include "Stream.h"

using namespace Napi;

using WorkerType = void *;

class FrameWorker : public AsyncProgressQueueWorker<WorkerType>
{
  public:
    FrameWorker(const Napi::CallbackInfo &info, std::deque<libcamera::Request *> *wait_queue, std::map<libcamera::Stream *, Napi::ObjectReference> *_stream_map);

    ~FrameWorker();

    void Execute(const ExecutionProgress &progress);

    void OnOK();

    void OnError(const Error &e);

    void OnProgress(const WorkerType *data, size_t /* count */);
    void stop();

    void notify();

  private:
    bool stopped = false;
    libcamera::Camera *camera;
    std::deque<libcamera::Request *> *cache_queue;
    std::mutex mtx; // 互斥量，保护产品缓冲区
    std::condition_variable frame_available;
    std::map<libcamera::Stream *, Napi::ObjectReference> *stream_map;
    std::map<libcamera::Stream *, unsigned int> *stream_index_map;
};
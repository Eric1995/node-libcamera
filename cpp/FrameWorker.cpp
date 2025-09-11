#include "FrameWorker.h"

FrameWorker::FrameWorker(const Napi::CallbackInfo &info, libcamera::Camera *_camera, std::deque<libcamera::Request *> *wait_queue, std::map<libcamera::Stream *, Stream *> *_stream_map,
                         std::map<libcamera::Stream *, unsigned int> *_stream_index_map)
    : AsyncProgressQueueWorker(info.Env()), camera(_camera), cache_queue(wait_queue), stream_map(_stream_map), stream_index_map(_stream_index_map)
{
}

FrameWorker::~FrameWorker()
{
}

void FrameWorker::Execute(const ExecutionProgress &progress)
{
    while (true)
    {
        std::unique_lock<std::mutex> lck(mtx);
        frame_available.wait(lck);
        progress.Signal();
        if (stopped)
        {
            break;
        }
    }
}

void FrameWorker::OnOK()
{
    HandleScope scope(Env());
}

void FrameWorker::OnError(const Error &e)
{
    HandleScope scope(Env());
}

void FrameWorker::OnProgress(const WorkerType *data, size_t /* count */)
{
    HandleScope scope(Env());
    while (cache_queue->size())
    {
        auto request = cache_queue->front();
        auto req_addr = Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(request));
        cache_queue->pop_front();
        for (auto &pair : request->buffers())
        {
            libcamera::Stream *stream = (libcamera::Stream *)pair.first;
            auto config = stream_map->at(stream);
            Napi::Object image = Image::constructor->New(
                {Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(stream)), Napi::BigInt::New(Env(), reinterpret_cast<uint64_t>(request)), Napi::Number::New(Env(), config->data_format)});
            if (stream_index_map->contains(stream))
            {
                unsigned int index = stream_index_map->at(stream);
                if (Stream::stream_config_map.contains(index))
                {
                    auto &callback = Stream::stream_config_map[index]->callback_ref;
                    if (callback && !callback.IsEmpty())
                    {
                        callback.Call({Env().Null(), Env().Null(), image});
                    }
                }
            }
        }
    }
}
void FrameWorker::stop()
{
    stopped = true;
    notify();
}

void FrameWorker::notify()
{
    frame_available.notify_all();
}

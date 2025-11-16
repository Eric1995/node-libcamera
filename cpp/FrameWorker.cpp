#include "FrameWorker.h"

FrameWorker::FrameWorker(const Napi::CallbackInfo &info, std::deque<libcamera::Request *> *wait_queue, std::map<libcamera::Stream *, Napi::ObjectReference> *_stream_map)
    : AsyncProgressQueueWorker(info.Env()), cache_queue(wait_queue), stream_map(_stream_map)
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
            break;
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
        cache_queue->pop_front();
        for (auto &pair : request->buffers())
        {
            auto stream = (libcamera::Stream *)pair.first;
            // 从 ObjectReference 获取 JS 对象，然后 Unwrap 获取 C++ 指针
            Napi::Object stream_obj = stream_map->at(stream).Value();
            auto mStream = Stream::Unwrap(stream_obj);
            auto external_stream = Napi::External<libcamera::Stream>::New(Env(), stream);
            auto external_request = Napi::External<libcamera::Request>::New(Env(), request);
            Napi::Object image = Image::constructor.New({external_stream, external_request});
            auto &callback = mStream->callback_ref;
            if (callback && !callback.IsEmpty())
                callback.Call({Env().Null(), Env().Null(), image});
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

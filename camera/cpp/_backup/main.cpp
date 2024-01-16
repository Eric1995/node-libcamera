#include <chrono>
#include <iostream>
#include <napi.h>
#include <thread>

using namespace std;

class MyWorker : public Napi::AsyncWorker
{
  public:
    MyWorker(Napi::Function &callback, int delay) : Napi::AsyncWorker(callback), delay_(delay)
    {
    }
    ~MyWorker()
    {
    }

    void Execute()
    {
        // std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
        // 处理耗时的任务，不能使用node-addon-api的api，只能用c++
    }

    void OnOK()
    {
        Napi::HandleScope scope(Env());

        Callback().Call({Env().Undefined()});
    }

  private:
    int delay_;
};

Napi::Value Delay(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Function callback = info[1].As<Napi::Function>();
    int delay = info[0].As<Napi::Number>().Int32Value();

    MyWorker *worker = new MyWorker(callback, delay);
    worker->Queue();

    return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("delay", Napi::Function::New(env, Delay));
    return exports;
}

NODE_API_MODULE(addon, Init)

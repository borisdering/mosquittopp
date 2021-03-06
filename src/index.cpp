#include "napi.h"
#include <string>
#include <iostream>
#include <vector>
#include "mosquitto.h"

struct mosquitto *mosq;

#define DEFAULT_QOS 0

class AsyncWorker : Napi::AsyncWorker
{
public:
    AsyncWorker(Napi::Function &callback)
        : Napi::AsyncWorker(callback) {}
    ~AsyncWorker() {}

    // Executed inside the worker-thread.
    // It is not safe to access JS engine data structure
    // here, so everything we need for input and output
    // should go on `this`.
    void Execute() { }

    void Execute(std::string topic, std::string message) {
        Callback().Call({Napi::String::New(Env(), topic), Napi::String::New(Env(), message)});
    }

    // Executed when the async work is complete
    // this function will be run inside the main event loop
    // so it is safe to use JS engine data again
    void OnOK()
    {
        // Callback().Call({Env().Undefined(), Napi::Number::New(Env(), estimate)});
    }
};

std::vector<AsyncWorker*> onMessageWorker;

void on_message(struct mosquitto *m, void *userdata, const struct mosquitto_message *message)
{
    for (const auto worker : onMessageWorker)
    {
        std::cout << ((char*) message->payload) << std::endl;
        std::cout << "NODE: MOSQUITTOPP: NEW MESSAGE TOPIC:'" << ((void*) message->topic) << "' MESSAGE:'" << ((void*) message->payload) << "'" << std::endl;
        worker->Execute(std::string((char*) message->topic), std::string((char*) message->payload));
    }
}

Napi::Number init(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    int result = mosquitto_lib_init();

    return Napi::Number::New(env, result);
}

Napi::Number connect(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    std::cout << "NODE: VERSION 0.0.5" << std::endl;

    Napi::String host = info[0].As<Napi::String>();
    Napi::Number port = info[1].As<Napi::Number>();

    // create a new mosquitto client instance.
    mosq = mosquitto_new(NULL, true, NULL);
    mosquitto_message_callback_set(mosq, on_message);
    int result = mosquitto_connect(mosq, std::string(host).c_str(), port.Int32Value(), 60);

    return Napi::Number::New(env, result);
}

void message(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    Napi::Function callback = info[0].As<Napi::Function>();

    onMessageWorker.push_back(new AsyncWorker(callback));
}

Napi::Number subscribe(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    Napi::String topic = info[0].As<Napi::String>();
    // Napi::Number qos = info[0].As<Napi::Number>();

    // missing qos,....

    std::cout << "NODE: MOSQUITTOPP: SUBSCRIBE TOPIC:'" << topic << "'" << std::endl;

    int result = mosquitto_subscribe(mosq, NULL, std::string(topic).c_str(), DEFAULT_QOS);

    return Napi::Number::New(env, result);
}

Napi::Number publish(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    Napi::String topic = info[0].As<Napi::String>();
    Napi::String payload = info[1].As<Napi::String>();
    Napi::Boolean retained = info[2].As<Napi::Boolean>();

    std::cout << "NODE: MOSQUITTOPP: PUBLISH TOPIC:'" << topic << "' MESSAGE:'" << payload << "'" << std::endl;

    std::string message = std::string(payload);

    int result = mosquitto_publish(mosq, NULL, std::string(topic).c_str(), message.length(), message.c_str(), DEFAULT_QOS, retained);

    return Napi::Number::New(env, result);
}

Napi::Number loop(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();

    int result = mosquitto_loop(mosq, -1, 1);

    return Napi::Number::New(env, result);
}

/**
 * 
 */
Napi::Object Init(Napi::Env env, Napi::Object exports)
{

    exports.Set(
        Napi::String::New(env, "mosquitto_init"),
        Napi::Function::New(env, init));

    exports.Set(
        Napi::String::New(env, "mosquitto_connect"),
        Napi::Function::New(env, connect));

    exports.Set(
        Napi::String::New(env, "mosquitto_subscribe"),
        Napi::Function::New(env, subscribe));

    exports.Set(
        Napi::String::New(env, "mosquitto_on_message"),
        Napi::Function::New(env, message));

    exports.Set(
        Napi::String::New(env, "mosquitto_publish"),
        Napi::Function::New(env, publish));

    exports.Set(
        Napi::String::New(env, "mosquitto_loop"),
        Napi::Function::New(env, loop));

    return exports;
}

NODE_API_MODULE(greet, Init);
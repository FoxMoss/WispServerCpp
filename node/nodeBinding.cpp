#include "../interface.hpp"
#include <cstdio>
#include <node.h>
#include <v8-primitive.h>
#include <v8.h>

using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Uint32;
using v8::Value;
Local<Context> context;
Local<Function> cb;
Isolate *isolate;

std::string ManuallyCopyString(char *data, size_t size) {
  std::string ret;
  for (size_t i = 0; i < size; i++) {
    ret += data[size];
  }
  return ret;
}
void sendCallback(void *data, size_t size, uint32_t id, bool exit) {
  const unsigned argc = 1;
  std::string dataString = ManuallyCopyString((char *)data, size);
  Local<Value> argv[argc] = {String::NewFromUtf8(isolate, (char *)data,
                                                 v8::NewStringType::kNormal,
                                                 size - 1)
                                 .ToLocalChecked()};

  cb->Call(context, Null(isolate), argc, argv).ToLocalChecked();
}
// args: id, sendcallback
void Open(const FunctionCallbackInfo<Value> &args) {
  isolate = args.GetIsolate();

  if (args.Length() < 2) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong number of arguments")
            .ToLocalChecked()));
    return;
  }

  if (!args[0]->IsUint32() && !args[1]->IsFunction()) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong argument").ToLocalChecked()));
    return;
  }

  context = isolate->GetCurrentContext();
  cb = Local<Function>::Cast(args[1]);

  open_interface(sendCallback, args[0].As<Uint32>()->Value());
}

void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "Open", Open);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

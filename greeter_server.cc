/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include "time.h"
#include <thread>
#include <chrono>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using grpc::Status;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
public:
  GreeterServiceImpl() {
    m_stream = nullptr;
    m_context = nullptr;
  }
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    std::cout<<"threadid="<<std::this_thread::get_id()<<" "<<prefix + request->name()<<std::endl;
    return Status::OK;
  }

  Status AsyncRequest(ServerContext* context, ServerReaderWriter<HelloReply, HelloRequest>* stream) override {
    HelloRequest note;
    m_stream = stream;
    m_context = context;
    while (stream->Read(&note)) {
      std::cout<<"threadid="<<std::this_thread::get_id()<<" "<<"recv="<<note.name()<<std::endl;
      std::unique_lock<std::mutex> lock(mu_);
      HelloReply resp;
      std::string info = "resp=";
      info += std::to_string(time(0));
      resp.set_message(info);
      stream->Write(resp);
    }
    std::cout<<"threadid="<<std::this_thread::get_id()<<" "<<"end AsyncRequest"<<std::endl;
    return Status::OK;
  }

  void WriteMsg(HelloReply& resp) {
    // 通道正常才能进行发送消息
    if(m_stream != nullptr && m_context != nullptr && !m_context->IsCancelled()) {
      m_stream->Write(resp);
    } else {
      std::cout<<"write failed."<<std::endl;
    }
  }

private:
  std::mutex mu_;  
  ServerContext* m_context;
  ServerReaderWriter<HelloReply, HelloRequest>* m_stream;
};

void RunServer() {
  std::string server_address("0.0.0.0:5001");
  GreeterServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  std::thread([&]()->void{
    while(true) {
      HelloReply resp;
      std::string info = "resp=";
      info += std::to_string(time(0));
      resp.set_message(info);
      service.WriteMsg(resp);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }).detach();

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
  
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}

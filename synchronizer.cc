#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include "database.h"
#include <grpc++/grpc++.h>


#define MAX_ROOM 10

#include "coordinator.grpc.pb.h"
#include "sns.grpc.pb.h"
#include "synchronizer.grpc.pb.h"



using grpc::Server;

using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;


using csce438::Message;
using syncer438::SynchronizerService;
using syncer438::Request;
using syncer438::Reply;
using syncer438::HeartBeat; 
using syncer438::RequesterType;



class SynchronizerServiceImpl final : public SynchronizerService::Service {
  
      
      Status checkFollowerUpdates(ServerContext* context, ServerReaderWriter<HeartBeat, HeartBeat>* stream) override {
          
        return Status::OK;

      }
      
};


void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SynchronizerServiceImpl service;
//hello
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {

    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "p:")) != -1){
        switch(opt) {
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    
    RunServer(port);
    
    return 0;
}




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
using coord438::CoordinatorService;

using syncer438::SynchronizerService;
using syncer438::Request;
using syncer438::Reply;
using syncer438::HeartBeat; 
using syncer438::RequesterType;


std::string serverType = "syncer";
std::string coordinatorIP = "localhost";
std::string coordinatorPort = "3000";
std::string syncer_id = "1";
std::string syncerPort = "8080";
std::unique_ptr<CoordinatorService::Stub> stubCoord_;

std::vector<std::string> all_clients;
std::vector<std::string> followers;


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


  std::string login_info = coordinatorIP + ":" + coordinatorPort;
  stubCoord_ = std::unique_ptr<CoordinatorService::Stub>(CoordinatorService::NewStub(
               grpc::CreateChannel(
                    login_info, grpc::InsecureChannelCredentials())));

  grpc::ClientContext context;
  coord438::Request request;
  coord438::Reply reply;
    
  //takes id from command line and sends it to coordinator
  request.set_id(stoi(syncer_id));
  request.set_port_number(syncerPort);
  request.set_server_type(serverType);

  grpc::Status status = stubCoord_->populateRoutingTable(&context, request, &reply);

  server->Wait();
}

int main(int argc, char** argv) {

    std::string coordinatorIP = "localhost";
    std::string coordinatorPort = "3000";
    std::string syncerId = "1";
    std::string syncerPort = "10000";
    int opt = 0;
    while ((opt = getopt(argc, argv, "i:c:p:d:")) != -1){
        switch(opt) {
            case 'i':
                coordinatorIP = optarg;break;
            case 'c':
                coordinatorPort = optarg;break;
            case 'p':
                syncerPort = optarg;break;
            case 'd':
                syncerId = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    
    RunServer(syncerPort);
    
    return 0;
}




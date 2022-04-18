#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <fstream>
#include "database.h"
#include <grpc++/grpc++.h>
#include <sys/stat.h>



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
  
      //update all clients files for the cluster
      Status checkAllClientsFiles(ServerContext* context, const Request* request, Reply* reply) override {
          


        return Status::OK;

      }
      
};


void checkForAllClientUpdates(std::string server_type, std::string server_id){

  std::cout << "I AM HERE3 " << std::endl;

  std::string dirname = server_type + "_" + server_id + "/all_clients.txt";
  struct stat result;
  if(stat(dirname.c_str(), &result)==0)
  {
      printf("old = %o", result.st_mtime);

  }

  while(1){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "I AM HERE" << std::endl;

    struct stat result2;
    if(stat(dirname.c_str(), &result2)==0)
    {
      printf("old = %o\n", result.st_mtime);

      printf("new = %o", result2.st_mtime);
      //this works

      if(result.st_mtime != result2.st_mtime){
        std::cout << "The file was altered" << std::endl;
        // an update was made to the all clients file
          // contact coordinator for something
          //grpc call to update the client files
          grpc::ClientContext context;
          coord438::AllClientsRequest request;
          coord438::AllClientsReply reply;
          std::string all_clients_in_cluster;

          std::fstream newfile;
          newfile.open(dirname,std::ios::in); //open a file to perform read operation using file object
          if (newfile.is_open()){   //checking whether the file is open
            std::string tp;
            while(getline(newfile, tp)){ //read data from file object and put it into string.
              all_clients_in_cluster.append(tp);
            }
          }
          std::cout  << "All clients in cluster string: " << all_clients_in_cluster << std::endl;
          request.set_all_clients_request(all_clients_in_cluster);
          //request.set_all_clients_request("HELLO");
          grpc::Status status = stubCoord_->getAllClients(&context, request, &reply);

          std::cout << "reply from syncer" << reply.all_clients_reply() << std::endl;
      }

    }
    
  }

}


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

  checkForAllClientUpdates("master", syncer_id);

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




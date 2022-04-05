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

using grpc::Server;

using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;



using coord438::CoordinatorService;
using coord438::Request;
using coord438::Reply;
using coord438::HeartBeat; 
using coord438::RequesterType;




/*


Develop the Coordinator C process which returns to a client the IP and port
number on which its Master runs. In the example above, the Coordinator
return the client c1 the IP/port for M1.

The coordinator implements a Centralized Algorithm for keeping track of
the IP/port for the Master server in each cluster. More implementation
details are below.

The Coordinator also keeps track of the Follower Synchronizer Fi IP/port
number in each cluster. More implementation details are below.
*/


std::vector<Servers> master_db;
std::vector<Servers> slave_db;
std::vector<Servers> followerSyncer_db;

class CoordinatorServiceImpl final : public CoordinatorService::Service {
  
      Status Login(ServerContext* context, const Request* request, Reply* reply) override {
          

        // first take client and assign is to a cluster (Xi) using mod3 formula
        // client assigned cluster is server id
        int server_id = (request->id()) % 3 + 1;

        //Check if any clients have joined and assign master port
        std::string port_num;
        if(master_db.size() == 0){
            port_num = "9190";
        } else{
            port_num = std::to_string(stoi(master_db[master_db.size()-1].portNum) + 100);
        }




        std::cout  << "server id: " << server_id << std::endl;
        std::cout  << "portNum : " << port_num << std::endl;

        
        //Create master
        Servers master;

        master.serverId = server_id;
        master.portNum = port_num;

        master_db.push_back(master);

        //f()
        
        // if(master_db[master.serverId].isActive = true){
        //     return Status::OK;
        // } else{
        //     return Status::OK;
        // }
        
        std::cout << "Coordinator Request ID: " << request->id() << std::endl;
        reply->set_port_number(port_num);
        reply->set_msg("Coordinator Login Successful\n");


        return Status::OK;
        
      }
      
      Status ServerCommunicate(ServerContext* context, ServerReaderWriter<HeartBeat, HeartBeat>* stream) override {
          
        return Status::OK;

        
      }
      
};


void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  CoordinatorServiceImpl service;
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




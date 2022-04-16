#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <chrono>
#include <time.h>
#include "database.h"
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"
#include "coordinator.grpc.pb.h"


#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>
#include <google/protobuf/duration.pb.h>


#define MAX_ROOM 10

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;


using csce438::Message;
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

//Vector for the routing tables
std::vector<Servers> master_db;
std::vector<Servers> slave_db;
std::vector<Servers> followerSyncer_db;
std::time_t temp_time;
std::time_t current_time;


int print_db(std::vector<Servers> db){
  for(Servers s : db){
    std::cout << "printing from db: " << s.serverId << std::endl;
    std::cout << "printing from db: "  << s.portNum << std::endl;

  }
  return -1;
}


std::string find_portNumber(std::string serverID, std::vector<Servers> db){
  int index = 0;
  for(Servers s : db){
    if(s.serverId == stoi(serverID))
      return s.portNum;
    index++;
  }
  return "error";
}

void displayTimestamp(std::time_t& time){
    std::string t_str(std::ctime(&time));
    t_str[t_str.size()-1] = '\0';
    std::cout << t_str << std::endl;
}

Servers findServer(std::string server_id){

    for(Servers s : master_db){
      if(s.serverId == stoi(server_id) && s.isActive == true){
        return s;
      } else if(s.serverId == stoi(server_id) && s.isActive == false){
          for(Servers s : slave_db){
            if(s.serverId == stoi(server_id)){
              return s;
            }
          }
      }
    }
  
}


class CoordinatorServiceImpl final : public CoordinatorService::Service {
  
      Status Login(ServerContext* context, const Request* request, Reply* reply) override {
          

        // first take client and assign is to a cluster (Xi) using mod3 formula
        // client assigned cluster is server id
        int server_id = (request->id() % 3) + 1;

        std::string server_type = request->server_type();

        print_db(master_db);

        google::protobuf::Timestamp timestamp = google::protobuf::Timestamp();
        timestamp.set_seconds(time(NULL));
        timestamp.set_nanos(0);


        current_time = google::protobuf::util::TimeUtil::TimestampToTimeT(timestamp);
        
        Servers s = findServer(std::to_string(server_id));

        std::cout << current_time - temp_time << std::endl;
        std::cout << "current time: " << current_time  << std::endl;
        std::cout << "server time: " << temp_time  << std::endl;


        if(current_time-s.timestamp > 30){
            s.isActive = false;
        }



        s = findServer(std::to_string(server_id));
            //should be s.portNum
        reply->set_port_number(s.portNum);
        reply->set_server_type(s.serverType);
        reply->set_server_id(std::to_string(server_id));
        //reply->set_port_number(find_portNumber(std::to_string(server_id), master_db));


        return Status::OK;
        
      }

      Status populateRoutingTable(ServerContext* context, const Request* request, Reply* reply) override {
        
        int server_id = request->id();
        std::string port_num;
        Servers serverInstance;

        if(request->server_type() == "master"){
          if(master_db.size() < 3){
            serverInstance.serverId = server_id;
            serverInstance.portNum = request->port_number();
            serverInstance.isActive = true;
            serverInstance.serverType = "master";
            master_db.push_back(serverInstance);
          }
        }else if (request->server_type() == "slave") {
          if(slave_db.size() < 3){
            serverInstance.serverId = server_id;
            serverInstance.portNum = request->port_number();
            serverInstance.isActive = true;
            serverInstance.serverType = "slave";

            slave_db.push_back(serverInstance);
          }
        }else if (request->server_type() == "syncer") {
            if(followerSyncer_db.size() < 3){
              serverInstance.serverId = server_id;
              serverInstance.portNum = request->port_number();
              serverInstance.isActive = true;
              serverInstance.serverType = "syncer";

              followerSyncer_db.push_back(serverInstance);
            }
        }
          
        print_db(followerSyncer_db);

        return Status::OK;
  
      }

      Status ServerCommunicate(ServerContext* context, ServerReaderWriter<HeartBeat, HeartBeat>* stream) override {
        //Communicate with server to check if master is still alive
        
        coord438::HeartBeat heartbeat;
        while(stream->Read(&heartbeat)) {
          //if timestamp in server instance - current time is greate than 30, set master status to false ans switch to slave
         
          Servers server = findServer(heartbeat.server_id());


          current_time = google::protobuf::util::TimeUtil::TimestampToTimeT(heartbeat.timestamp());
          temp_time = current_time;

          std::cout << "Testing heartbeat functionality: " << heartbeat.server_id() << std::endl;
          std::cout << "blach: " << current_time << std::endl;
          //std::cout << "lehrgey: " << server.timestamp << std::endl;

          displayTimestamp(current_time);

        }

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




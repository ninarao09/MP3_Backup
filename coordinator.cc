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
#include "synchronizer.grpc.pb.h"



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
std::vector<std::string> allClients_db;
std::time_t temp_time;
std::time_t current_time;
std::time_t old_time;



void print_db(std::vector<Servers> db){
  for(Servers s : db){
    std::cout << "printing from db - id: " << s.serverId << std::endl;
    std::cout << "printing from db - port: "  << s.portNum << std::endl;
    std::cout << "printing from db - time: "  << s.timestamp << std::endl;
  }
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
  
  if(master_db[stoi(server_id)-1].isActive = true){
    master_db[stoi(server_id)-1].serverType = "master";
    return master_db[stoi(server_id)-1];
  } else if(master_db[stoi(server_id)-1].isActive = false){
    slave_db[stoi(server_id)-1].serverType = "slave";
    return slave_db[stoi(server_id)-1];
  }

}


Servers* findServer2(std::string server_id){
  
  if(master_db[stoi(server_id)-1].isActive = true){
    master_db[stoi(server_id)-1].serverType = "master";
    return &master_db[stoi(server_id)-1];
  } else if(master_db[stoi(server_id)-1].isActive = false){
    slave_db[stoi(server_id)-1].serverType = "slave";
    return &slave_db[stoi(server_id)-1];
  }

}

// Servers getMasterOrSlave(bool status){
//     if(status == true){

//     }
// }

int findServerIndex(std::string server_id, std::string server_type){
  std::vector<Servers> db;
  if(server_type == "master"){
      db = master_db;
  }else{
      db = slave_db;
  }

  int count = 0;
  for(Servers s: db){
    if(s.serverId == stoi(server_id)){
      return count;
    }
    count++;
  }
  return -1;
}

class CoordinatorServiceImpl final : public CoordinatorService::Service {
  
      Status Login(ServerContext* context, const Request* request, Reply* reply) override {
          

        // first take client and assign is to a cluster (Xi) using mod3 formula
        // client assigned cluster is server id
        int server_id = (request->id() % 3) + 1;

        //std::string server_type = request->server_type();
        Servers* server = findServer2(std::to_string(server_id));
        //Servers* s = &server;
          std::cout << "server type returned is - before: " << server->serverType << std::endl;
          std::cout << "server port returned is - before: " << server->portNum << std::endl;
          std::cout << "server id returned is - before: " << server->serverId << std::endl;

        google::protobuf::Timestamp timestamp = google::protobuf::Timestamp();
        timestamp.set_seconds(time(NULL));
        timestamp.set_nanos(0);
        current_time = google::protobuf::util::TimeUtil::TimestampToTimeT(timestamp);
        // std::cout << "server timestamp in login : " << s.timestamp << std::endl;
        // std::cout << "current timestamp in login : " << current_time << std::endl;


          //if time change is greater than 2 missing heartbeats
        if(current_time - server->timestamp > 20){
            std::cout << "Server was down for more than 20" << std::endl;
            server->isActive = false;
        }

            //server->isActive = false;

        //Set the server type
        Servers s = findServer(std::to_string(server_id));




          std::cout << "server type returned is: " << s.serverType << std::endl;
          std::cout << "server port returned is: " << s.portNum << std::endl;
          std::cout << "server id returned is: " << s.serverId << std::endl;
          std::cout << "server status returned is: " << server->isActive << std::endl;



            //should be s.portNum
        reply->set_port_number(server->portNum);
        reply->set_server_type(server->serverType);
        reply->set_server_id(std::to_string(server_id));
        //reply->set_port_number(find_portNumber(std::to_string(server_id), master_db));


        return Status::OK;
        
      }

      Status populateRoutingTable(ServerContext* context, const Request* request, Reply* reply) override {
        
        int server_id = request->id();
        std::string port_num;
        Servers serverInstance;

        google::protobuf::Timestamp timestamp = google::protobuf::Timestamp();
        timestamp.set_seconds(time(NULL));
        timestamp.set_nanos(0);
        current_time = google::protobuf::util::TimeUtil::TimestampToTimeT(timestamp);


        if(request->server_type() == "master"){
          if(master_db.size() < 3){
            serverInstance.serverId = server_id;
            serverInstance.portNum = request->port_number();
            serverInstance.isActive = true;
            serverInstance.serverType = "master";
            serverInstance.timestamp = current_time;
            master_db.push_back(serverInstance);
          }
        }else if (request->server_type() == "slave") {
          if(slave_db.size() < 3){
            serverInstance.serverId = server_id;
            serverInstance.portNum = request->port_number();
            serverInstance.isActive = true;
            serverInstance.serverType = "slave";
            serverInstance.timestamp = current_time;
            slave_db.push_back(serverInstance);
          }
        }else if (request->server_type() == "syncer") {
            if(followerSyncer_db.size() < 3){
              serverInstance.serverId = server_id;
              serverInstance.portNum = request->port_number();
              serverInstance.isActive = true;
              serverInstance.serverType = "syncer";
              //serverInstance.timestamp = current_time;
              followerSyncer_db.push_back(serverInstance);
            }
        }
          
        //print_db(master_db);
        //print_db(slave_db);


        return Status::OK;
  
      }


      Status getAllClients(ServerContext* context, const Request* request, Reply* reply) override {

        std::cout << "Called From SYNCRONIZER: " << std::endl;

        //std::cout << "From SYNCRONIZER: " << request->all_clients_request() << std::endl;
        //push those values into all clients db

        //reply->set_all_clients_reply("pls work");
        return Status::OK;
      }

      Status ServerCommunicate(ServerContext* context, ServerReaderWriter<HeartBeat, HeartBeat>* stream) override {
        //Communicate with server to check if master is still alive

        coord438::HeartBeat heartbeat;
        while(stream->Read(&heartbeat)) {
          //if timestamp in server instance - current time is greate than 30, set master status to false ans switch to slave
         
          
          int server_index = findServerIndex(heartbeat.server_id(), heartbeat.s_type());

          Servers* server;
          if(heartbeat.s_type()=="master"){
            server = &master_db[server_index];
          }else if(heartbeat.s_type()=="slave"){
            server = &slave_db[server_index];

          }
         

          current_time = google::protobuf::util::TimeUtil::TimestampToTimeT(heartbeat.timestamp());
          std::cout << "server = before : " << server->timestamp << std::endl;
          std::cout << "current before :  " << current_time << std::endl;
          
          //f time change is le
          if(current_time - server->timestamp > 20){
              std::cout << "Server was down for more than 20 seconds - server commincate function" << std::endl;
              server->isActive = false;
          }


          std::cout << "Testing heartbeat functionality: " << heartbeat.server_id() << heartbeat.s_type() << std::endl;
          //std::cout << "current after :  " << current_time << std::endl;
        
          server->timestamp = current_time;
          std::cout << "server after = : " << server->timestamp << std::endl;

          //print_db(master_db);
          //(slave_db);


          displayTimestamp(current_time);

        }
        //server.timestamp = current_time;

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




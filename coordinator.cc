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

struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client that has been created
std::vector<Client> client_db;


std::vector<Servers> master_db;
std::vector<Servers> slave_db;
std::vector<Servers> followerSyncer_db;

//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

int print_db(std::vector<Servers> db){
  int index = 0;
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

class CoordinatorServiceImpl final : public CoordinatorService::Service {
  
      Status Login(ServerContext* context, const Request* request, Reply* reply) override {
          
          Client c;
          std::string username = std::to_string(request->id());
          int user_index = find_user(username);
          if(user_index < 0){
            c.username = username;
            client_db.push_back(c);
            reply->set_msg("Login Successful!");
          }
          else{ 
            Client *user = &client_db[user_index];
            if(user->connected)
              reply->set_msg("Invalid Username");
            else{
              std::string msg = "Welcome Back " + user->username;
              reply->set_msg(msg);
              user->connected = true;
            }
          }
          

        // first take client and assign is to a cluster (Xi) using mod3 formula
        // client assigned cluster is server id
        int server_id = (request->id()) % 3 + 1;
        std::string port_num;

        //Assign master port - dummy
        // std::string port_num;
        // if(server_id == 1){
        //   port_num = request->port_number();
        // }else if(server_id == 2){
        //   port_num = std::to_string(stoi(request->port_number()) + 100);
        // }else if(server_id == 3){
        //   port_num = std::to_string(stoi(request->port_number()) + 200);
        // }


        Servers serverInstance;
        if(request->server_type() == "master"){
          if(master_db.size() < 3){
            serverInstance.serverId = server_id;
            serverInstance.portNum = request->port_number();
            serverInstance.isActive = true;
            master_db.push_back(serverInstance);
          } else{
            port_num = find_portNumber(std::to_string(server_id), master_db);
            reply->set_port_number(port_num);
            return Status::OK;
          }

        } else if(request->server_type() == "slave"){

        } else{

        }

        print_db(master_db);



        std::cout  << "server id: " << server_id << std::endl;
        std::cout  << "portNum : " << request->port_number() << std::endl;
        std::cout  << "serverType : " << request->server_type() << std::endl;
        std::cout << "Coordinator Request ID: " << request->id() << std::endl;

        reply->set_port_number(request->port_number());


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




/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <memory>
#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <chrono>
#include "database.h"

#include "sns.grpc.pb.h"
#include "coordinator.grpc.pb.h"


using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;
using coord438::CoordinatorService;
using coord438::HeartBeat;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;



std::string serverType = "master";
std::string coordinatorIP = "localhost";
std::string coordinatorPort = "3000";
std::string id = "1";
std::string clientPort = "8080";
std::unique_ptr<CoordinatorService::Stub> stubCoord_;
std::unique_ptr<SNSService::Stub> stubSlave_;


bool isMaster = false;

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

struct Client2{
  std::string username;
  ServerReaderWriter<Message, Message>* stream = 0;
};

//Vector that stores every client that has been created
std::vector<Client> client_db;

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

int find_userFromCluster(std::string username){
  int index = 0;
  std::fstream newfile2;
    std::string dirname2 = "master_" + id + "/all_clients.txt"; 
    newfile2.open(dirname2,std::ios::app|std::ios::in|std::ios::out); //open a file to perform read operation using file object
    if (newfile2.is_open()){   //checking whether the file is open
      std::string tp;
      while(getline(newfile2, tp)){ //read data from file object and put it into string.
        if(username == tp){
          return index;
        }
        index++;
      }
    }
    newfile2.close();
  return -1;
}

void addClientToFile(std::string server_type, std::string server_id, std::string client_id, std::string filename){
  std::string dirname = server_type +  "_" + server_id;
  std::string fileinput = "/" + filename;
  std::ofstream outputfile(dirname+fileinput, std::ios::app|std::ios::out|std::ios::in);
  outputfile<<client_id<<std::endl;
}

void createDirectories(std::string server_type, std::string server_id){
  int dir;
  std::string dirname = server_type + "_" + server_id;
  struct stat buffer;

  if(stat(dirname.c_str(), &buffer) == 0){

    std::string fileinput = "/all_clients.txt";
    std::ofstream outputfile(dirname+fileinput, std::ios::app|std::ios::out|std::ios::in);
    std::string fileinput2 = "/total_clients.txt";
    std::ofstream outputfile2(dirname+fileinput2, std::ios::app|std::ios::out|std::ios::in);

  } else{

    dir = mkdir(dirname.c_str(),0777);
    if(!dir){
      std::cout << "Directory created" << std::endl;
    }else{
      std::cout << "Error creating directory" << std::endl;

    }

    std::string fileinput = "/all_clients.txt";
    std::ofstream outputfile(dirname+fileinput, std::ios::app|std::ios::out|std::ios::in);
    
    std::string fileinput2 = "/total_clients.txt";
    std::ofstream outputfile2(dirname+fileinput2, std::ios::app|std::ios::out|std::ios::in);
  }

    

  
}

void sendHeartbeat(const std::string &id) {
    
    grpc::ClientContext context;

    std::shared_ptr<ClientReaderWriter<HeartBeat, HeartBeat>> stream(
            stubCoord_->ServerCommunicate(&context));

    //Thread used to read chat messages and send them to the server
    std::thread writer([id, stream]() {
            HeartBeat m;
            m.set_server_id(id);
            m.set_s_type(serverType);

            

            google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
            timestamp->set_seconds(time(NULL));
            timestamp->set_nanos(0);
            m.set_allocated_timestamp(timestamp);

            stream->Write(m);
              while (1) {
                std::this_thread::sleep_for(std::chrono::seconds(10));

                google::protobuf::Timestamp* timestamp2 = new google::protobuf::Timestamp();
                timestamp2->set_seconds(time(NULL));
                timestamp2->set_nanos(0);
                m.set_allocated_timestamp(timestamp2);

                stream->Write(m);
              }
            stream->WritesDone();
            });

    writer.join();
}



class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    std::cout << "I am in the list function server side" << std::endl;
    
    int index = 0;

    std::fstream newfile2;
    std::string dirname2 = "master_" + id + "/total_clients.txt"; 
    newfile2.open(dirname2,std::ios::app|std::ios::in|std::ios::out); //open a file to perform read operation using file object
    if (newfile2.is_open()){   //checking whether the file is open
      std::string tp;
      while(getline(newfile2, tp)){ //read data from file object and put it into string.
        list_reply->add_all_users(tp);
      }
    }
    newfile2.close();


    //std::string server_id = std::to_string((stoi(id)%3)+1);
    std::fstream newfile;
    std::string dirname = "master_" + id + "/" + request->username() + "_followers.txt"; 
    std::cout << "client int list: " <<request->username() << std::endl;
    newfile.open(dirname,std::ios::app|std::ios::in|std::ios::out); //open a file to perform read operation using file object
    if (newfile.is_open()){   //checking whether the file is open
      std::string tp;
      while(getline(newfile, tp)){ //read data from file object and put it into string.
        list_reply->add_followers(tp);
      }
    }
    newfile.close();


    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);

    std::string s_type;
    if(isMaster==true){
        s_type = "master";
    }else{
        s_type = "slave";
    }

    int join_index = find_user(username2);
    if(username1 == username2)
      reply->set_msg("unkown user name");
    else{


      reply->set_msg("Follow Successful");

      addClientToFile(s_type, id,  username2, username1 + "_following.txt");
      //addClientToFile("slave", id, username1, username2 + "_followers.txt");
      if(isMaster==true){
        Request request2;
        ClientContext context2;
        Reply reply2;


        request2.set_username(request->username());
        request2.add_arguments(request->arguments(0));

        //std::string slave_port = fi
        Status status2 = stubSlave_->Follow(&context2, request2, &reply2);

      }


    }
    return Status::OK; 
  }

  Status getServerPort(ServerContext* context, const Request* request, Reply* reply) override {
    reply->set_msg(clientPort + "," + serverType);

    return Status::OK; 
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    Client c;
    std::string username = request->username();

    std::string s_type;

    if(isMaster==true){
        s_type = "master";
    }else{
        s_type = "slave";
    }

    //check if user was created already
    std::fstream newfile;
    std::string dirname = s_type + "_" + id + "/all_clients.txt";
    newfile.open(dirname,std::ios::in|std::ios::out); //open a file to perform read operation using file object
    if (newfile.is_open()){   //checking whether the file is open
      std::string tp;
      while(getline(newfile, tp)){ //read data from file object and put it into string.
        if(tp == username){
          return Status::OK;
        }
      }
      newfile.close(); //close the file object.
    }


    int user_index = find_user(username);
    if(user_index < 0){
      c.username = username;
      client_db.push_back(c);
      reply->set_msg("Login Successful!");
      addClientToFile(s_type, id, username, "all_clients.txt");
      addClientToFile(s_type, id, username, "total_clients.txt");

      std::string dirname2 = s_type + "_" + id;
      std::string fileinput2 = "/" + username + "_following.txt";
      std::string fileinput3 = "/" +  username + "_followers.txt";
      std::ofstream outputfile2(dirname2+fileinput2, std::ios::app|std::ios::out|std::ios::in);
      std::ofstream outputfile3(dirname2+fileinput3, std::ios::app|std::ios::out|std::ios::in);
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

    if(isMaster==true){
      Request request2;
      ClientContext context2;
      Reply reply2;

      request2.set_username(username);
      request2.set_server_type("slave");

      //std::string slave_port = fi
      Status status2 = stubSlave_->Login(&context2, request2, &reply2);

    }

    

    return Status::OK;
  }

  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {

    std::string s_type;
    if(isMaster==true){
        s_type = "master";
    }else{
        s_type = "slave";
    }

     //when I run a server again I need to populate the client db with the all clients file 
    



    

    Message message;
    Client *c;

    std::string user, line;
    std::vector<std::string> followers;
    std::vector<std::string> messages;


    while(stream->Read(&message)) {
      std::string username = message.username();

      std::fstream newfile2;
      std::string dirname2 = s_type + "_" + id + "/all_clients.txt"; 
      newfile2.open(dirname2,std::ios::app|std::ios::in|std::ios::out); //open a file to perform read operation using file object
      if (newfile2.is_open()){   //checking whether the file is open
        std::string tp;
        while(getline(newfile2, tp)){ //read data from file object and put it into string.
            Client client;
            client.username = username;
            client.stream = 0;
            client_db.push_back(client);
        }
      }
      newfile2.close();

      int user_index = find_user(username);
      c = &client_db[user_index];
 
      //Write the current message to "username.txt"
      std::string dirname = s_type+"_"+id+"/";
      std::string filename = username+"_t.txt";
      std::ofstream user_file(dirname+filename,std::ios::app|std::ios::out|std::ios::in);
      std::string fileinput = message.username()+" > "+message.msg()+"\n";
      //"Set Stream" is the default message from the client to initialize the stream
      if(message.msg() != "Set Stream")
        user_file << fileinput;
      //If message = "Set Stream", print the first 20 chats from the people you follow
      else{
        if(c->stream==0){
      	  c->stream = stream;
        }
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(dirname+username+"_actualtimeline.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        while(getline(in, line)){
          if(c->following_file_size > 20){
            if(count < c->following_file_size-20){
              count++;
              continue;
            }
          }
          newest_twenty.push_back(line);
        }
        Message new_msg; 
 	      //Send the newest messages to the client to be displayed
	      for(int i = 0; i<newest_twenty.size(); i++){
	        new_msg.set_msg(newest_twenty[i]);
          stream->Write(new_msg);
        }    
        continue;
      }
      // //Send the message to each follower's stream
      std::vector<Client*>::const_iterator it;
      for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
        Client *temp_client = *it;
      	if(temp_client->stream!=0 && temp_client->connected)
	      temp_client->stream->Write(message);
        //For each of the current user's followers, put the message in their following.txt file
        std::string temp_username = temp_client->username;
        std::string temp_file = temp_username + "_actualtimeline.txt";
	      std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
	      following_file << fileinput;
        temp_client->following_file_size++;
	      std::ofstream user_file(temp_username + ".txt",std::ios::app|std::ios::out|std::ios::in);
        user_file << fileinput;
      }

      std::fstream newfile;
      newfile.open(dirname,std::ios::app|std::ios::in|std::ios::out); //open a file to perform read operation using file object
      if (newfile.is_open()){   //checking whether the file is open
        std::string tp;
        while(getline(newfile, tp)){ //read data from file object and put it into string.
          int client_index = find_user(tp);
          Client* temp_client = &client_db[client_index];
          if(temp_client->stream!=0 && temp_client->connected){
	          temp_client->stream->Write(message);
          }
          std::string temp_username = temp_client->username;
          std::string temp_file = temp_username + "_actualtimeline.txt";
          std::ofstream following_file(dirname+temp_file,std::ios::app|std::ios::out|std::ios::in);
          following_file << fileinput;
          temp_client->following_file_size++;
          std::ofstream user_file(dirname+temp_username+"_t.txt",std::ios::app|std::ios::out|std::ios::in);
          user_file << fileinput;
        }
      }
      newfile.close();
    }
    //If the client disconnected from Chat Mode, set connected to false
    c->connected = false;

    if(isMaster==true){
      ClientContext context;
      std::shared_ptr<ClientReaderWriter<Message, Message>> stream(
            stubSlave_->Timeline(&context));

    }

    return Status::OK;
  }

};

void getSlaveInfo(std::string server_id){
  //call frunction from coordinator to get slave port

  ClientContext context;
  coord438::Request request;
  coord438::Reply reply;

  request.set_id(stoi(server_id));
  Status status = stubCoord_->getSlaveInfo(&context, request, &reply);
  std::cout << "SLAVE PORT :::: " << reply.slave_port() << std::endl;

  std::string login_info = "localhost:" + reply.slave_port();

  stubSlave_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
                grpc::CreateChannel(
                      login_info, grpc::InsecureChannelCredentials())));


}




void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  //here I should push into the master db or call the get server functon

    grpc::ClientContext context;
    coord438::Request request;
    coord438::Reply reply;
      
    //takes id from command line and sends it to coordinator
    request.set_id(stoi(id));
    request.set_port_number(clientPort);
    request.set_server_type(serverType);

    grpc::Status status = stubCoord_->populateRoutingTable(&context, request, &reply);

  //here I should populate total_clients db

  // make directory of Type and id storing all the context file
  
  createDirectories(serverType, id);
  if(serverType == "master"){
    isMaster = true;

    //call coordinator to get slave stub

    getSlaveInfo(id);
    
  }

  sendHeartbeat(id);

  server->Wait();
}

int main(int argc, char** argv) {
  

  int opt = 0;
  while ((opt = getopt(argc, argv, "i:c:p:d:t:")) != -1){
    switch(opt) {
      case 'i':
                coordinatorIP = optarg;break;
      case 'c':
                coordinatorPort = optarg;break;
      case 'p':
                clientPort = optarg;break;
      case 'd':
                id = optarg;break;
      case 't':
                serverType = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }

  std::string login_info = "localhost:" + coordinatorPort;

    stubCoord_ = std::unique_ptr<CoordinatorService::Stub>(CoordinatorService::NewStub(
                grpc::CreateChannel(
                      login_info, grpc::InsecureChannelCredentials())));

  
   





  RunServer(clientPort);

  

  return 0;
}




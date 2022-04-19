
//**************************************************************************************************************************************************

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

#include "synchronizer.grpc.pb.h"
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

using sync438::Request;
using sync438::Reply;
using sync438::SynchronizerService;
using coord438::CoordinatorService;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;



std::string serverType = "syncer";
std::string coordinatorIP = "localhost";
std::string coordinatorPort = "3000";
std::string id = "1";
std::string clientPort = "8080";
std::unique_ptr<CoordinatorService::Stub> stubCoord_;


class SynchronizerServiceImpl final : public SynchronizerService::Service {
  
  Status checkAllClientsFiles(ServerContext* context, const Request* request, Reply* list_reply) override {
 

    return Status::OK;
  }

    Status checkAllFollowerFiles(ServerContext* context, const Request* request, Reply* list_reply) override {
 

    return Status::OK;
  }



};


void checkForAllClientUpdates(std::string server_type, std::string server_id){

  std::cout << "I AM HERE3 " << std::endl;

  std::string dirname = server_type + "_" + server_id + "/all_clients.txt";
  struct stat result;
  if(stat(dirname.c_str(), &result)==0)
  {
      printf("old = %lo", result.st_mtime);

  }

  while(1){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "I AM HERE" << std::endl;

    struct stat result2;
    if(stat(dirname.c_str(), &result2)==0)
    {
      printf("old = %lo\n", result.st_mtime);

      printf("new = %lo", result2.st_mtime);
      //this works

      if(result.st_mtime != result2.st_mtime){
        std::cout << "The file was altered" << std::endl;
        // an update was made to the all clients file
          // contact coordinator for something
          //grpc call to update the client files
          grpc::ClientContext context;
          coord438::Request request;
          coord438::Reply reply;
          std::string all_clients_in_cluster;

          std::fstream newfile;
          newfile.open(dirname,std::ios::in|std::ios::out); //open a file to perform read operation using file object
          if (newfile.is_open()){   //checking whether the file is open
            std::string tp;
            while(getline(newfile, tp)){ //read data from file object and put it into string.
              all_clients_in_cluster.append(tp);
              all_clients_in_cluster.append(".");

            }
          }
          newfile.close();
          std::cout  << "All clients in cluster string: " << all_clients_in_cluster << std::endl;
          request.set_all_clients_request(all_clients_in_cluster);
          std::cout <<"req: " << request.all_clients_request() << std::endl;
          grpc::Status status1 = stubCoord_->getAllClients(&context, request, &reply);

          std::cout << "All clients reply: " << reply.all_clients_reply() << std::endl;


      }

    }
    
  }
}

void checkForFollowerUpdates(std::string server_type, std::string server_id){

  //monitor every follower file in the cluster
  //if there is a change in this cluster
  //then update the other FS clusters of who is  following the person in the other cluster

}


void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SynchronizerServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  //here I should push into the master db or call the get server functon
  std::string login_info = "localhost:" + coordinatorPort;

  stubCoord_ = std::unique_ptr<CoordinatorService::Stub>(CoordinatorService::NewStub(
               grpc::CreateChannel(
                    login_info, grpc::InsecureChannelCredentials())));

  grpc::ClientContext context;
  coord438::Request request;
  coord438::Reply reply;
    
  //takes id from command line and sends it to coordinator
  request.set_id(stoi(id));
  request.set_port_number(clientPort);
  request.set_server_type(serverType);

  grpc::Status status = stubCoord_->populateRoutingTable(&context, request, &reply);

  if(status.ok()){
    std::cout << "somehting is right" << std::endl;

  }else {
    std::cout << "somehting is wrong" << std::endl;
  }

  checkForAllClientUpdates("master", id);
  //checkForAllClientUpdates("slave", id);


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
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }



  RunServer(clientPort);

  

  return 0;
}






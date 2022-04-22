
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
#include <experimental/filesystem>
#include <thread>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <chrono>
#include "dirent.h"
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
using sync438::HeartBeat;
using sync438::FollowerRequest;
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
std::unique_ptr<SynchronizerService::Stub> stubFS1_;
std::unique_ptr<SynchronizerService::Stub> stubFS2_;

long int old_time;

struct otherSyncers{
  std::string server_id_1;
  std::string port_num_1;
  std::string server_id_2;
  std::string port_num_2;
};

otherSyncers syncer;

struct oldFileTimes{
  std::string filename;
  long int old_time;
};

std::vector<oldFileTimes> old_file_times;


class SynchronizerServiceImpl final : public SynchronizerService::Service {


  Status sendFollowerInfo(ServerContext* context, const FollowerRequest* request, Reply* reply) override {

      //update follower file in the respective cluster





    return Status::OK;
  }

   Status coordinatorCommunicate(ServerContext* context, ServerReaderWriter<HeartBeat, HeartBeat>* stream) override {
      return Status::OK;
   }

};



int findOldTimeIndex(std::string filename){
  int index=0;
  for(oldFileTimes o : old_file_times){
    if(o.filename == filename){
      return index;
    }
    index++;
  }
  return -1;
}

void ifTheFileWasAllClients(std::string server_id){

  std::string dirname = "master_" + server_id + "/all_clients.txt";

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

          //get all clients in that exist - send the new user to the database
          std::cout  << "All clients in cluster string: " << all_clients_in_cluster << std::endl;
          request.set_all_clients_request(all_clients_in_cluster);
          std::cout <<"req: " << request.all_clients_request() << std::endl;
          grpc::Status status1 = stubCoord_->getAllClients(&context, request, &reply);
          std::cout << "All clients reply: " << reply.all_clients_reply() << std::endl;


          //contact coordinator to recieve other fs ports.
            // grpc::ClientContext context2;
            // coord438::Request request2;
            // coord438::FSReply reply2;
            // request2.set_id(stoi(id));
            // request2.set_port_number(clientPort);
            // request2.set_server_type(serverType);
            // grpc::Status status2 = stubCoord_->getFSServerInfo(&context2, request2, &reply2);

            // std::cout << "other id 1 " << reply2.id_1() << std::endl;
            // std::cout << "other port 1 " << reply2.port_number_1() << std::endl;
            // std::cout << "other id 2 " << reply2.id_2() << std::endl;
            // std::cout << "other port 2 " << reply2.port_number_2() << std::endl;
            // syncer.server_id_1 = reply2.id_1();
            // syncer.port_num_1 = reply2.port_number_1();
            // syncer.server_id_2 = reply2.id_2();
            // syncer.port_num_1 = reply2.port_number_2();

}


void populateOldTimeDB(std::string server_id){

  //old_file_times.clear();

  DIR *dir2 = nullptr;
  struct dirent *ent2 = nullptr;
  std::string dirname = "master_" + server_id;

    dir2 = opendir (("master_" + server_id).c_str());

    if (dir2 != nullptr) {
      // looping through cluster directory for every file
      while (ent2 = readdir (dir2)) {

        
          if( !strcmp(ent2->d_name, ".") || !strcmp(ent2->d_name, "..")){
             //djfh
          }else{

            struct stat initial;
            if(stat((dirname + "/" + ent2->d_name).c_str(), &initial)==0){
              oldFileTimes oldFile;

              oldFile.filename = ent2->d_name;
              oldFile.old_time = initial.st_mtime;

              old_file_times.push_back(oldFile);
            }
          }

        
      }
      closedir (dir2);


    } else {
      /* could not open directory */
      perror ("could not open directory");
    }

}

void checkForUpdates(std::string server_type, std::string server_id){

  //monitor every follower file in the cluster
  //if there is a change in this cluster
  //then update the other FS clusters of who is  following the person in the other cluster
    DIR *dir = nullptr;
    struct dirent *ent = nullptr;
    std::string dirname = "master_" + server_id;

  populateOldTimeDB(server_id);
  for(oldFileTimes o : old_file_times){
    std::cout << o.filename << ":" << o.old_time <<std::endl;
  }


  while(1){
    std::this_thread::sleep_for(std::chrono::seconds(5));

    
    std::cout <<"I am running : " << std::endl;



    DIR *dir;
    struct dirent *ent;

    dir = opendir (("master_" + server_id).c_str());

    if (dir != nullptr) {
      // looping through cluster directory for every file
      while (ent = readdir (dir)) {
        //std::cout << "I AME HEREREEEEE 4" << std::endl;

        if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")){
          //ksjdhf
        }else{

          int index = findOldTimeIndex(ent->d_name);
          


          //std::cout << "index: " << index << std::endl;
          std::cout << "filname: " << ent->d_name << std::endl;

          

            struct stat result;
            if(stat((dirname + "/" + ent->d_name).c_str(), &result)==0){

              if(index == -1){
                //populateOldTimeDB(server_id);
                //push the new entries into the db
                oldFileTimes old;
                old.filename = ent->d_name;
                old.old_time = result.st_mtime;
                old_file_times.push_back(old);
              } 

              oldFileTimes* o = &old_file_times[index];

              if(o->old_time  != result.st_mtime){
                  std::cout<< ent->d_name << " was modified :::::: " << "old time: " << o->old_time << ", new file time: " << result.st_mtime << std::endl;
                  if(strcmp(ent->d_name, "all_clients.txt")==0){
                    std::cout << "I am iin the all clients if" << std::endl;
                    //here  I should update the old files db again    
                  }
                  //call the stub function so it can  contact the other FS to update of its follower info

                  o->old_time = result.st_mtime;
              }            
            }else{
              std::cout << "error in stat vall for: " << ent->d_name <<std::endl;
            }

          
          

        }
      }
      closedir (dir);
    } else {
      perror ("could not open directory");
    }
  }

}

void populateRoutingTable(){

  grpc::ClientContext context;
  coord438::Request request;
  coord438::Reply reply;
    
  //takes id from command line and sends it to coordinator
  request.set_id(stoi(id));
  request.set_port_number(clientPort);
  request.set_server_type(serverType);

  grpc::Status status = stubCoord_->populateRoutingTable(&context, request, &reply);

}


void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SynchronizerServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  std::string login_info = "localhost:" + coordinatorPort;
  stubCoord_ = std::unique_ptr<CoordinatorService::Stub>(CoordinatorService::NewStub(
               grpc::CreateChannel(
                    login_info, grpc::InsecureChannelCredentials())));

  

  populateRoutingTable();
  checkForUpdates("master", id);
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






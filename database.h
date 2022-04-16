#ifndef DATABASE_H_
#define DATABASE_H_
#include <ctype.h>
#include "client.h"
#include <time.h>






struct Servers{
  int serverId;
  std::string portNum;
  bool isActive = false;
  time_t timestamp;
  std::string serverType;
};


// class Common {
//   public:
//     static std::vector<Servers> master_db;
// };

#endif
#ifndef DATABASE_H_
#define DATABASE_H_
#include <ctype.h>
#include "client.h"
#include <time.h>






struct Servers{
  int serverId;
  std::string portNum;
  bool isActive = true;
  time_t timestamp;
  std::string serverType;
  std::string stubName;
};


// class Common {
//   public:
//     static std::vector<Servers> master_db;
// };

#endif
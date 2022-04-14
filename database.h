#ifndef DATABASE_H_
#define DATABASE_H_
#include <ctype.h>
#include "client.h"





struct Servers{
  int serverId;
  std::string portNum;
  bool isActive = false;
  std::string timestamp;
};


// class Common {
//   public:
//     static std::vector<Servers> master_db;
// };

#endif
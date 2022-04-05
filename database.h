#ifndef DATABASE_H_
#define DATABASE_H_
#include <ctype.h>


struct Servers{
  int serverId;
  std::string portNum;
  bool isActive = false;
};

#endif
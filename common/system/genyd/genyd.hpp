#ifndef GENYD_HPP_
#define GENYD_HPP_

#include <map>

#include <unistd.h>

#include "socket.hpp"

class Genyd {

public:
  Genyd(void);
  ~Genyd(void);

private:
  Genyd(const Genyd &);
  Genyd operator=(const Genyd &);

private:
  Socket *server;
  std::map<int, Socket *> clients;

  int setFS(fd_set *readfs, fd_set *writefs, fd_set *exceptfs) const;
  void acceptNewClient(void);

public:
  void run(void);
  bool isInit(void) const;

};

#endif

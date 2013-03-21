#ifndef GENYD_HPP_
#define GENYD_HPP_

#include <map>

#include <unistd.h>

#include "global.hpp"
#include "socket.hpp"
#include "dispatcher.hpp"

class Genyd {

public:
  Genyd(void);
  ~Genyd(void);

private:
  Genyd(const Genyd &);
  Genyd operator=(const Genyd &);

private:
  Socket *server;
  Dispatcher dispatcher;
  std::map<int, Socket *> clients;

  int setFS(fd_set *readfs, fd_set *writefs, fd_set *exceptfs) const;
  void acceptNewClient(void);
  void treatMessage(Socket::ReadStatus status, Socket *client);

public:
  void run(void);
  bool isInit(void) const;

};

#endif

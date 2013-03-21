#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <sstream>

#include "global.hpp"

class Socket {

public:
  enum ReadStatus {
    Error,
    NoMessage,
    NewMessage
  };

  Socket(int socket);
  ~Socket(void);

private:
  Socket(void);
  Socket(const Socket &);
  Socket operator=(const Socket &);

private:
  int socket;
  Request request;

  std::stringstream istream;
  std::stringstream ostream;

public:
  ReadStatus read(void);
  void write(const char *data);
  int getFD(void) const;
  const Request &getRequest(void) const;
};


#endif

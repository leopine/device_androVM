#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <queue>
#include <sstream>

#include "global.hpp"

class Socket {

public:
  enum ReadStatus {
    ReadError,
    NoMessage,
    NewMessage
  };
  enum WriteStatus {
    WriteError,
    BadSerialize,
    WriteSuccess
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
  std::queue<Reply *> replies;

public:
  ReadStatus read(void);
  bool hasReplies(void) const;
  WriteStatus reply(void);
  void write(const char *data);
  int getFD(void) const;
  const Request &getRequest(void) const;
  void addReply(Reply *reply);
};


#endif

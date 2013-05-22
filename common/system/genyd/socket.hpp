#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <queue>
#include <sstream>

#include "global.hpp"

/*
** C socket encapsulation
*/
class Socket {

public:
  enum ReadStatus {
    ReadError,
    NoMessage,
    NewMessage,
    UnknownMessage,
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
  // Socket fd
  int socket;
  // Request to treat
  Request request;
  // Datastream (fed by socket buffer and consume by Protobuf)
  std::stringstream istream;
  // Repleis list
  std::queue<Reply *> replies;

public:
  // Read data from socket
  ReadStatus read(void);

  // Return whether there're replies to send or not
  bool hasReplies(void) const;

  // Try and write a reply on the socket
  WriteStatus reply(void);

  // Get the socket fd
  int getFD(void) const;

  // Get the current request to treat
  const Request &getRequest(void) const;

  // Add a reply to the replies list
  void addReply(Reply *reply);
};


#endif

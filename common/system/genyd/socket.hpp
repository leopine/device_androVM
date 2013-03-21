#ifndef SOCKET_HPP_
#define SOCKET_HPP_

class Socket {

public:
  Socket(int socket);
  ~Socket(void);

private:
  Socket(void);
  Socket(const Socket &);
  Socket operator=(const Socket &);

private:
  int socket;

public:
  char *read(void);
  void write(const char *data);
  int getFD(void) const;

};


#endif

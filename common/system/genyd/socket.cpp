
#include <cutils/log.h>

#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>

#include "socket.hpp"


Socket::Socket(int socket)
  : socket(socket)
{

}

Socket::~Socket(void)
{
  if (socket != -1)
    close(socket);
}

char *Socket::read(void)
{
  int len = 0;
  char *cmd = 0;
  char buffer[1024];

  if ((len = recv(socket, buffer, 1023, 0)) < 0)
    {
      SLOGE("recv() error");
    }

  if (len <= 0)
    {
      return 0;
    }

  cmd = new char[len + 1];
  bcopy(buffer, cmd, len * sizeof(*cmd));

  return cmd;
}

void Socket::write(const char *data)
{
  (void)data;
}

int Socket::getFD(void) const
{
  return (socket);
}

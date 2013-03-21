
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>

#include "genyd.hpp"

#define SERVER_PORT 22666

Genyd::Genyd(void)
  : server(NULL)
{
  int server_sock = -1;
  struct sockaddr_in server_addr;

  bzero(&server_addr, sizeof(server_addr));

  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);

  if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    SLOGE("Cannot create socket");
    return;
  }

  if (bind(server_sock, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    SLOGE("Cannot bind socket");
    return;
  }

  if (listen(server_sock, 5) == -1) {
    SLOGE("Cannot listen on socket");
    return;
  }
  server = new Socket(server_sock);
}

Genyd::~Genyd(void)
{
  if (server) {
    delete server;
  }

  std::map<int, Socket*>::iterator begin = clients.begin();
  std::map<int, Socket*>::iterator end = clients.end();

  while (begin != end) {
    delete begin->second;
    ++begin;
  }
}

int Genyd::setFS(fd_set *readfs, fd_set *writefs, fd_set *exceptfs) const
{
  int highest = server->getFD();

  std::map<int, Socket*>::const_iterator begin = clients.begin();
  std::map<int, Socket*>::const_iterator end = clients.end();

  FD_ZERO(readfs);
  FD_ZERO(writefs);
  FD_ZERO(exceptfs);

  FD_SET(server->getFD(), readfs);
  //FD_SET(server->getFD(), exceptfs);

  while (begin != end) {
    FD_SET(begin->first, readfs);
    //FD_SET(begin->first, exceptfs);
    highest = std::max(highest, begin->first);
    ++begin;
  }

  return highest;
}

void Genyd::acceptNewClient(void)
{
  int client = -1;
  sockaddr_in clientAddr;
  int clientAddrSize = sizeof(clientAddr);

  client = accept(server->getFD(), (sockaddr *)&clientAddr, &clientAddrSize);

  if (client == -1)
    {
      SLOGE("Cannot accept connection");
      return;
    }

  clients[client] = new Socket(client);
  SLOGI("New client connected", clients.size());
}

void Genyd::treatMessage(Socket::ReadStatus status, Socket *client)
{
  if (status == Socket::NoMessage) {
    return;
  }
  const Request &request = client->getRequest();
  SLOGD("Request type is %d", request.type());
}

void Genyd::run(void)
{
  fd_set readfs, writefs, exceptfs;

  while (true) {
    int highest = setFS(&readfs, &writefs, &exceptfs);

    if (select(highest + 1, &readfs, &writefs, &exceptfs, NULL) < 0) {
      SLOGE("select() error");
      return;
    }

    if (FD_ISSET(server->getFD(), &readfs)) {
      SLOGD("Can accept new client");
      acceptNewClient();
    }

    std::map<int, Socket*>::iterator begin = clients.begin();
    std::map<int, Socket*>::iterator end = clients.end();

    while (begin != end) {
      if (FD_ISSET(begin->first, &readfs)) {
	SLOGD("Client %d id ready-read", begin->first);

	Socket::ReadStatus status = begin->second->read();

	if (status == Socket::Error) {
	  delete begin->second;
	  clients.erase(begin++);
	  continue;
	} else {
	  treatMessage(status, begin->second);
	}
      }

      if (FD_ISSET(begin->first, &writefs)) {
	SLOGD("Client %d id ready-write", begin->first);
	begin->second->write("OK\n");
      }

      ++begin;
    }
  }
}

bool Genyd::isInit(void) const
{
  return (server);
}

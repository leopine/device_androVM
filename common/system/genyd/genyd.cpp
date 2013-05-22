
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

int Genyd::setFS(fd_set *readfs, fd_set *writefs) const
{
    int highest = server->getFD();

    std::map<int, Socket*>::const_iterator begin = clients.begin();
    std::map<int, Socket*>::const_iterator end = clients.end();

    FD_ZERO(readfs);
    FD_ZERO(writefs);

    // For "accept()"
    FD_SET(server->getFD(), readfs);

    while (begin != end) {
        FD_SET(begin->first, readfs);
        if (begin->second->hasReplies()) {
            FD_SET(begin->first, writefs);
        }
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

    if (client == -1) {
        SLOGE("Cannot accept connection");
        return;
    }

    clients[client] = new Socket(client);
}

void Genyd::treatMessage(Socket *client)
{
    const Request &request = client->getRequest();
    client->addReply(dispatcher.dispatchRequest(request));
}

void Genyd::run(void)
{
    fd_set readfs;
    fd_set writefs;

    // Server loop
    while (true) {
        int highest = setFS(&readfs, &writefs);

        // Wait for operations
        if (select(highest + 1, &readfs, &writefs, NULL, NULL) < 0) {
            SLOGE("select() error");
            return;
        }

        // Accept new connection
        if (FD_ISSET(server->getFD(), &readfs)) {
            acceptNewClient();
        }

        std::map<int, Socket*>::iterator begin = clients.begin();
        std::map<int, Socket*>::iterator end = clients.end();

        // Handle messages for clients
        while (begin != end) {
            // Ready read
            if (FD_ISSET(begin->first, &readfs)) {
                Socket::ReadStatus status = begin->second->read();
                switch (status) {
                case Socket::ReadError:
                    SLOGD("Socket read error");
                case Socket::NoMessage:
                    delete begin->second;
                    clients.erase(begin++);
                    continue;
                    break;
                case Socket::NewMessage:
                case Socket::UnknownMessage:
                    treatMessage(begin->second);
                    break;
                default:
                    SLOGE("Unknown socket status");
                }
            }

            // Ready write
            if (FD_ISSET(begin->first, &writefs)) {
                Socket::WriteStatus status = begin->second->reply();
                if (status == Socket::WriteError) {
                    SLOGD("Socket write error with client %d", begin->first);
                    delete begin->second;
                    clients.erase(begin++);
                    continue;
                }
            }

            ++begin;
        }
    }
}

bool Genyd::isInit(void) const
{
    return (server);
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>

#include "server.h"
#include "global.h"

int create_listening_socket(int port, uint32_t ip)
{
    int s;
    struct sockaddr_in sockaddr;
    bzero(&sockaddr, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = htonl(ip);

    s = socket(AF_INET, SOCK_STREAM, 0) ;
    if (s < 0) {
        SLOGE("Unable to create android hw side socket: %d (%s)\n",
               errno, strerror(errno));
        return -1;
    }

    /* make sure port is reuse if not previously closed */
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    /* Enable TCP NO DELAY */
    int opt_nodelay;
    opt_nodelay = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt_nodelay, sizeof(opt_nodelay));

    /* Bind to socket*/
    if (bind(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        SLOGE("Unable to bind socket: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    /* listen for hw connection */
    if (listen(s, LISTEN_BACKLOG) < 0) {
        SLOGE("Unable to listen on socket: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return s;
}

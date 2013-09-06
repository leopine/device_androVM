#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "buffer.h"
#include "server.h"
#include "global.h"

/**
 * local_camera: is a simple proxy server which listen for 2 clients:
 *     o the Genymotion player that will handle the real webcam
 *     o Android framework that will try to connect to the webcam via TCP
 *
 * [Android] --- 127.0.0.1:HW_PORT - [ local_camera ] - 0.0.0.0:PLAYER_PORT ---- [Genymotion]
 *
 * the first one to connect is the Android side, which connect on boot, then it waits for
 * Genymotion player to connect.
 *
 * When the two part are connected, messages are forwarded between them without any modifications
 *  o If connection is lost from the player, we listen again for its connection.
 *  o If connection is lost from the android side, we exit local_camera, and Android will restart it.
 *
 */


int main(int argc, char *argv[])
{
    int player_sock, hw_sock, hw_fd, c;
    int player_port = DEFAULT_PLAYER_PORT;
    int hw_port = DEFAULT_LOCAL_PORT;

    /* read listening ports from arguments if any */
    opterr = 0;
    while ((c = getopt (argc, argv, "l:p:h")) != -1) {
        switch (c) {
        case 'l':
            hw_port = atoi(optarg);
            break;
        case 'p':
            player_port = atoi(optarg);
            break;
        case 'h':
        default:
            fprintf(stderr, "Options are -l [local port for android] -p [player port]");
            return 0;
        }
    }

    /* initialize listening socket, Android side */
    hw_sock = create_listening_socket(hw_port, INADDR_LOOPBACK);
    if (hw_sock < 0) {
        SLOGE("Unable to create android hw side socket");
        return -1;
    }

    /* Accept connection from android */
    hw_fd = accept(hw_sock, NULL, 0);
    if (hw_fd < 0) {
        SLOGE("Failed to connect hw client %d (%s)", errno, strerror(errno));
        return -1;
    }
    LOGD("Hw connected fd:%d", hw_fd);
    /* Enable TCP NO DELAY */
    int opt_nodelay;
    opt_nodelay = 1;
    setsockopt(hw_fd, IPPROTO_TCP, TCP_NODELAY, &opt_nodelay, sizeof(opt_nodelay));

    /* now that Android is connected, listen for the player connection */
    player_sock = create_listening_socket(player_port, INADDR_ANY);
    if (player_sock < 0) {
        SLOGE("Unable to create player socket");
        return -1;
    }

    /* Now wait for the hw to connect/disconnect/reconnect etc. */
    while (1) {
        struct sockaddr_in player_cli_addr;
        bzero(&player_cli_addr, sizeof(struct sockaddr_in));
        socklen_t player_cli_addr_len = 0;
        int player_fd, max_fd;

        LOGD("Waiting for player");
        player_fd = accept(player_sock, (struct sockaddr *)&player_cli_addr,
                           &player_cli_addr_len);
        if (player_fd < 0) {
            SLOGE("Failed to connect player: %d (%s)", errno, strerror(errno));
            return -1;
        }
        LOGD("Player connected fd:%d", player_fd);

        /* Enable TCP NO DELAY */
        setsockopt(player_fd, IPPROTO_TCP, TCP_NODELAY, &opt_nodelay, sizeof(opt_nodelay));

        fd_set rfds;
        char read_buffer[READ_BUFFER_SIZE + 1];
        int read_len = 0;

        /* the two sides are connected, forward every request from A to B and
           from B to A */
        max_fd = (player_fd > hw_fd ? player_fd : hw_fd) + 1;
        FD_ZERO(&rfds);

        /* add the two socket to the read monitoring */
        FD_SET(player_fd, &rfds);
        FD_SET(hw_fd, &rfds);

        while (select(max_fd, &rfds, NULL, NULL, NULL) > -1) {
            if (FD_ISSET(player_fd, &rfds)) {
                LOGD("Data from player");
                /* read data coming from the player */
                read_len = read(player_fd, read_buffer, sizeof(read_buffer));

                if (read_len <= 0) {
                    SLOGE("We have a problem with the player connection %d (%s)",
                           errno, strerror(errno));
                    break;
                } else {
                    /* write hw msg to the player */
                    LOGD("Writing msg to hw: (%.*s)", read_buffer, read_len);

                    int wr = send(hw_fd, read_buffer, read_len, MSG_NOSIGNAL);
                    if (wr <= 0) {
                        SLOGE("Failed to write data to hw cnx %d (%s)",
                              errno, strerror(errno));
                        return -1;;
                    }
                }
            }
            if (FD_ISSET(hw_fd, &rfds)) {
                LOGD("Data from hw");
                /* read data coming from the hw */
                read_len = read(hw_fd, read_buffer, sizeof(read_buffer));

                if (read_len <= 0) {
                    SLOGE("We have a problem with the hw connection %d (%s)",
                           errno, strerror(errno));
                    return 1;
                } else {
                    LOGD("Writing msg to player: (%.*s)", read_buffer, read_len);

                    int wr = send(player_fd, read_buffer, read_len, MSG_NOSIGNAL);
                    if (wr <= 0) {
                        SLOGE("Failed to write data to player cnx %d (%s)",
                              errno, strerror(errno));
                        return -1;
                    }
                }
            }

            /* continue to monitor socket on reading */
            FD_ZERO(&rfds);
            FD_SET(player_fd, &rfds);
            FD_SET(hw_fd, &rfds);
        }
        close(player_fd);
    }
    close(hw_fd);

    return 0;
}

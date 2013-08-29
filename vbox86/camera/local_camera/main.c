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

    /* handle our 2 reception buffers */
    buffer_t player_buffer;
    buffer_t hw_buffer;

    if (init_buffer(&player_buffer, "player buffer")) {
        SLOGE("Failed to initialize player buffer");
        return 1;
    }
    if (init_buffer(&hw_buffer, "android hw buffer")) {
        SLOGE("Failed to initialize player buffer");
        return 1;
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

        fd_set rfds, wfds;
        char read_buffer[READ_BUFFER_SIZE + 1];
        int read_len;

        /* the two sides are connected, forward every request from A to B and
           from B to A */
        max_fd = (player_fd > hw_fd ? player_fd : hw_fd) + 1;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        /* add the two socket to the read monitoring */
        FD_SET(player_fd, &rfds);
        FD_SET(hw_fd, &rfds);

        while (select(max_fd, &rfds, &wfds, NULL, NULL) > -1) {
            /* if we have to write data from the player to the hw,
               check that the socket is writable */
            if ((player_buffer.len > 0) && FD_ISSET(hw_fd, &wfds)) {
                /* write hw msg to the player */
                LOGD("Writing msg to hw: (%.*s)", player_buffer.len,
                       player_buffer.p_start);

                int wr = send(hw_fd, player_buffer.p_start,
                              player_buffer.len, MSG_NOSIGNAL);
                if (wr <= 0) {
                    SLOGE("Failed to write data to hw cnx %d (%s)",
                           errno, strerror(errno));
                    /* TODO: we might reset the connection in this case */
                } else if (wr < player_buffer.len) {
                    /* remove data sent from buffer */
                    remove_from_buffer(&player_buffer, wr);
                    LOGD("Data pending for player: %d bytes",
                           player_buffer.len);
                } else {
                    /* reset buffer now every thing has been read */
                    empty_buffer(&player_buffer);
                }
            }
            /* if we have to write data from hw to the player,
               check that the socket is writable */
            if ((hw_buffer.len > 0) && FD_ISSET(player_fd, &wfds)) {
                /* write hw msg to the player */
                LOGD("Writing msg to player: (%.*s)", hw_buffer.len,
                       hw_buffer.p_start);
                int wr = send(player_fd, hw_buffer.p_start, hw_buffer.len,
                              MSG_NOSIGNAL);
                if (wr <= 0) {
                    SLOGE("Failed to write data to player cnx %d (%s)",
                           errno, strerror(errno));
                    /* TODO: we might reset the connection in this case */
                } else if (wr < hw_buffer.len) {
                    /* remove data sent from buffer */
                    remove_from_buffer(&hw_buffer, wr);
                    LOGD("Data pending for hw (%d bytes)",
                           hw_buffer.len);
                } else {
                    /* TODO resize buffer in this case */
                    /* reset buffer now every thing has been read */
                    empty_buffer(&hw_buffer);
                }
            }

            if (FD_ISSET(player_fd, &rfds)) {
                /* read data coming from the player */
                read_len = read(player_fd, read_buffer, sizeof(read_buffer));

                if (read_len <= 0) {
                    /* TODO */
                    SLOGE("We have a problem with the player connection %d (%s)",
                           errno, strerror(errno));
                    return -1;
                }

                /* we need to bufferize data */
                if (add_to_buffer(&player_buffer, read_buffer, read_len)) {
                    SLOGE("Failed to bufferize data on player socket");
                    /* TODO */
                    return 1;
                }

                LOGD("We have bufferized (player side): (%.*s)",
                      player_buffer.len, player_buffer.p_start);
            }
            if (FD_ISSET(hw_fd, &rfds)) {
                /* read data coming from the hw */
                read_len = read(hw_fd, read_buffer, sizeof(read_buffer));

                if (read_len <= 0) {
                    SLOGE("We have a problem with the hw connection %d (%s)",
                           errno, strerror(errno));
                    break;
                }

                /* we need to bufferize data */
                if (add_to_buffer(&hw_buffer, read_buffer, read_len)) {
                    SLOGE("Failed to bufferize data on player socket");
                    /* TODO */
                    return 1;
                }

                LOGD("We have bufferized (hw side): (%.*s)",
                      hw_buffer.len, hw_buffer.p_start);
            }

            /* continue to monitor socket on reading */
            FD_ZERO(&rfds);
            FD_SET(player_fd, &rfds);
            FD_SET(hw_fd, &rfds);

            /* monitor write fd_set if one of the buffer has data */
            FD_ZERO(&wfds);
            /* player needs to talk to hw */
            if (player_buffer.len > 0) FD_SET(hw_fd, &wfds);
            /* hw needs to talk to player */
            if (hw_buffer.len > 0) FD_SET(player_fd, &wfds);
        }
        close(player_fd);
    }
    close(hw_fd);

    delete_buffer(&player_buffer);
    delete_buffer(&hw_buffer);

    return 0;
}

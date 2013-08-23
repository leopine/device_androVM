#ifndef LOCAL_CAMERA_SERVER_H
#define LOCAL_CAMERA_SERVER_H

#define PLAYER_PORT 24800
#define HW_PORT 24801
#define LISTEN_BACKLOG  1
/* TODO: ^ might need more slot if connection are not closed properly */

/**
 * @brief Create a listening socket, binded on specified ip/port
 * @return the socket number, -1 on error
 */
int create_listening_socket(int port, uint32_t ip);

#endif /* LOCAL_CAMERA_SERVER_H */

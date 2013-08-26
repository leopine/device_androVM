#ifndef LOCAL_CAMERA_SERVER_H
#define LOCAL_CAMERA_SERVER_H

#define BACKCAMERA_PLAYER_PORT 24800
#define BACKCAMERA_LOCAL_PORT 24801

#define FRONTCAMERA_PLAYER_PORT 24810
#define FRONTCAMERA_LOCAL_PORT 24811

#define DEFAULT_PLAYER_PORT BACKCAMERA_PLAYER_PORT
#define DEFAULT_LOCAL_PORT  BACKCAMERA_LOCAL_PORT

#define LISTEN_BACKLOG  1
/* TODO: ^ might need more slot if connection are not closed properly */

/**
 * @brief Create a listening socket, binded on specified ip/port
 * @return the socket number, -1 on error
 */
int create_listening_socket(int port, uint32_t ip);

#endif /* LOCAL_CAMERA_SERVER_H */

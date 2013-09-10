#ifndef LOCAL_SERVER_GLOBAL_H
#define LOCAL_SERVER_GLOBAL_H

#define LOG_TAG "local_camera"
#include <cutils/log.h>

#define DEBUG_CAMERA 0
#if DEBUG_CAMERA
#define LOGD(...)    SLOGD(__VA_ARGS__)
#else
#define LOGD(...)    ((void)0)
#endif /* DEBUG_CAMERA */

#define READ_BUFFER_SIZE (640*480*12)
#endif /* LOCAL_SERVER_GLOBAL_H */

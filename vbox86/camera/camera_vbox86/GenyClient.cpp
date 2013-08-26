/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains implementation of classes that encapsulate connection to camera
 * services in the emulator via local_camera srv socket.
 */

#define LOG_NDEBUG 1
#define LOG_TAG "EmulatedCamera_GenyClient"
#include <cutils/log.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "EmulatedCamera.h"
#include "GenyClient.h"

#define LOG_QUERIES 0
#if LOG_QUERIES
#define LOGQ(...)   ALOGD(__VA_ARGS__)
#else
#define LOGQ(...)   (void(0))

#endif  // LOG_QUERIES
namespace android {

/****************************************************************************
 * Geny client base
 ***************************************************************************/

GenyClient::GenyClient()
    : mSocketFD(-1)
{
}

GenyClient::~GenyClient()
{
    if (mSocketFD >= 0) {
        close(mSocketFD);
    }
}

/****************************************************************************
 * Geny client API
 ***************************************************************************/

status_t GenyClient::connectClient(const int local_srv_port)
{
    struct sockaddr_in so_addr;
    int fds = -1;
    ALOGV("%s: port %d", __FUNCTION__, local_srv_port);

    /* Make sure that client is not connected already. */
    if (mSocketFD >= 0) {
        ALOGE("%s: Geny client is already connected", __FUNCTION__);
        return EINVAL;
    }

    /* Connect to the local_camera server */
    fds = socket(AF_INET, SOCK_STREAM, 0);
    if (fds < 0) {
        ALOGE("%s: Unable to create socket to the camera service port %d: %s",
             __FUNCTION__, local_srv_port, strerror(errno));
        return errno ? errno : EINVAL;
    }

    bzero(&so_addr, sizeof(so_addr));
    so_addr.sin_family = AF_INET;
    so_addr.sin_port = htons(local_srv_port);
    so_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    mSocketFD = connect(fds, (struct sockaddr *)&so_addr, sizeof(so_addr));
    if (mSocketFD < 0) {
        ALOGE("%s: Unable to connect to the camera service port %d: %s",
             __FUNCTION__, local_srv_port, strerror(errno));
        return errno ? errno : EINVAL;
    }

    return NO_ERROR;
}

void GenyClient::disconnectClient()
{
    ALOGV("%s", __FUNCTION__);

    if (mSocketFD >= 0) {
        close(mSocketFD);
        mSocketFD = -1;
    }
}

status_t GenyClient::sendMessage(const void* data, size_t data_size)
{
    if (mSocketFD < 0) {
        ALOGE("%s: Geny client is not connected", __FUNCTION__);
        return EINVAL;
    }

    LOGQ("Sending '%.*s'", data_size, data);

    int wr_res = write(mSocketFD, data, data_size);
    if (wr_res != data_size) {
        ALOGE("%s: Unable to write message %d (size=%d): %s",
              __FUNCTION__, wr_res, data_size, strerror(errno));
        return errno ? errno : EIO;
    }
    return NO_ERROR;
}

status_t GenyClient::receiveMessage(void** data, size_t* data_size)
{
    *data = NULL;
    *data_size = 0;

    if (mSocketFD < 0) {
        ALOGE("%s: Geny client is not connected", __FUNCTION__);
        return EINVAL;
    }

    /* The way the service replies to a query, it sends payload size first, and
     * then it sends the payload itself. Note that payload size is sent as a
     * string, containing 8 characters representing a hexadecimal payload size
     * value. Note also, that the string doesn't contain zero-terminator. */
    size_t payload_size;
    char payload_size_str[9];

    int rd_res = read(mSocketFD, payload_size_str, 8);
    if (rd_res != 8) {
        ALOGE("%s: Unable to obtain payload size: %s",
             __FUNCTION__, strerror(errno));
        return errno ? errno : EIO;
    }
    LOGQ("Received payload '%s'", payload_size_str);

    /* Convert payload size. */
    errno = 0;
    payload_size_str[8] = '\0';
    payload_size = strtol(payload_size_str, NULL, 16);
    if (errno) {
        ALOGE("%s: Invalid payload size '%s'", __FUNCTION__, payload_size_str);
        return EIO;
    }

    /* Allocate payload data buffer, and read the payload there. */
    *data = malloc(payload_size);
    if (*data == NULL) {
        ALOGE("%s: Unable to allocate %d bytes payload buffer",
             __FUNCTION__, payload_size);
        return ENOMEM;
    }
    rd_res = read(mSocketFD, *data, payload_size);
    if (static_cast<size_t>(rd_res) == payload_size) {
        *data_size = payload_size;
        return NO_ERROR;
    } else {
        ALOGE("%s: Read size %d doesnt match expected payload size %d: %s",
             __FUNCTION__, rd_res, payload_size, strerror(errno));
        free(*data);
        *data = NULL;
        return errno ? errno : EIO;
    }
}

status_t GenyClient::doQuery(QemuQuery* query)
{
    /* Make sure that query has been successfuly constructed. */
    if (query->mQueryDeliveryStatus != NO_ERROR) {
        ALOGE("%s: Query is invalid", __FUNCTION__);
        return query->mQueryDeliveryStatus;
    }

    LOGQ("Send query '%s'", query->mQuery);

    /* Send the query. */
    status_t res = sendMessage(query->mQuery, strlen(query->mQuery) + 1);
    if (res == NO_ERROR) {
        /* Read the response. */
        res = receiveMessage(reinterpret_cast<void**>(&query->mReplyBuffer),
                      &query->mReplySize);
        if (res == NO_ERROR) {
            LOGQ("Response to query '%s': Status = '%.2s', %d bytes in response",
                 query->mQuery, query->mReplyBuffer, query->mReplySize);
        } else {
            ALOGE("%s Response to query '%s' has failed: %s",
                 __FUNCTION__, query->mQuery, strerror(res));
        }
    } else {
        ALOGE("%s: Send query '%s' failed: %s",
             __FUNCTION__, query->mQuery, strerror(res));
    }

    /* Complete the query, and return its completion handling status. */
    const status_t res1 = query->completeQuery(res);
    ALOGE_IF(res1 != NO_ERROR && res1 != res,
            "%s: Error %d in query '%s' completion",
            __FUNCTION__, res1, query->mQuery);
    return res1;
}


/****************************************************************************
 * Geny client for an 'emulated camera' service.
 ***************************************************************************/

/*
 * Emulated camera queries
 */

/* Connect to the camera device. */
const char CameraGenyClient::mQueryConnect[]    = "connect";
/* Disconnect from the camera device. */
const char CameraGenyClient::mQueryDisconnect[] = "disconnect";
/* Query info from the webcam. */
const char CameraGenyClient::mQueryInfo[]      = "infos";
/* Start capturing video from the camera device. */
const char CameraGenyClient::mQueryStart[]      = "start";
/* Stop capturing video from the camera device. */
const char CameraGenyClient::mQueryStop[]       = "stop";
/* Get next video frame from the camera device. */
const char CameraGenyClient::mQueryFrame[]      = "frame";

CameraGenyClient::CameraGenyClient()
    : GenyClient()
{
}

CameraGenyClient::~CameraGenyClient()
{

}

status_t CameraGenyClient::queryConnect()
{
    ALOGV("%s", __FUNCTION__);

    QemuQuery query(mQueryConnect);
    doQuery(&query);
    const status_t res = query.getCompletionStatus();
    ALOGE_IF(res != NO_ERROR, "%s: Query failed: %s",
            __FUNCTION__, query.mReplyData ? query.mReplyData :
                                             "No error message");
    return res;
}

status_t CameraGenyClient::queryDisconnect()
{
    ALOGV("%s", __FUNCTION__);

    QemuQuery query(mQueryDisconnect);
    doQuery(&query);
    const status_t res = query.getCompletionStatus();
    ALOGE_IF(res != NO_ERROR, "%s: Query failed: %s",
            __FUNCTION__, query.mReplyData ? query.mReplyData :
                                             "No error message");
    return res;
}

status_t CameraGenyClient::queryInfo(uint32_t *p_pixel_format,
                                      int *p_width,
                                      int *p_height)
{
    ALOGV("%s", __FUNCTION__);

    if (!p_pixel_format || !p_width || !p_height) {
        ALOGE("%s: invalid parameter", __FUNCTION__);
    }

    QemuQuery query(mQueryInfo);
    if (doQuery(&query) || !query.isQuerySucceeded()) {
        ALOGE("%s: Camera info query failed: %s", __FUNCTION__,
             query.mReplyData ? query.mReplyData : "No error message");
        return query.getCompletionStatus();
    }

    /* Make sure there is info returned. */
    if (query.mReplyDataSize == 0) {
        ALOGE("%s: No camera info returned.", __FUNCTION__);
        return EINVAL;
    }

    /* TODO: parse info string */

    ALOGE("Parsing not implemented");
    memcpy(p_pixel_format, "RGBA", 4);
    *p_width = 640;
    *p_height = 480;
    return NO_ERROR;
}

status_t CameraGenyClient::queryStart(uint32_t pixel_format,
                                      int width,
                                      int height)
{
    ALOGV("%s", __FUNCTION__);

    char query_str[256];
    snprintf(query_str, sizeof(query_str), "%s dim=%dx%d pix=%d",
             mQueryStart, width, height, pixel_format);
    QemuQuery query(query_str);
    doQuery(&query);
    const status_t res = query.getCompletionStatus();
    ALOGE_IF(res != NO_ERROR, "%s: Query failed: %s",
            __FUNCTION__, query.mReplyData ? query.mReplyData :
                                             "No error message");
    return res;
}

status_t CameraGenyClient::queryStop()
{
    ALOGV("%s", __FUNCTION__);

    QemuQuery query(mQueryStop);
    doQuery(&query);
    const status_t res = query.getCompletionStatus();
    ALOGE_IF(res != NO_ERROR, "%s: Query failed: %s",
            __FUNCTION__, query.mReplyData ? query.mReplyData :
                                             "No error message");
    return res;
}

status_t CameraGenyClient::queryFrame(void* vframe,
                                      void* pframe,
                                      size_t vframe_size,
                                      size_t pframe_size,
                                      float r_scale,
                                      float g_scale,
                                      float b_scale,
                                      float exposure_comp)
{
    ALOGV("%s", __FUNCTION__);

    char query_str[256];
    snprintf(query_str, sizeof(query_str), "%s video=%d preview=%d whiteb=%g,%g,%g expcomp=%g",
             mQueryFrame, (vframe && vframe_size) ? vframe_size : 0,
             (pframe && pframe_size) ? pframe_size : 0, r_scale, g_scale, b_scale,
             exposure_comp);
    QemuQuery query(query_str);
    doQuery(&query);
    const status_t res = query.getCompletionStatus();
    if( res != NO_ERROR) {
        ALOGE("%s: Query failed: %s",
             __FUNCTION__, query.mReplyData ? query.mReplyData :
                                              "No error message");
        return res;
    }

    /* Copy requested frames. */
    size_t cur_offset = 0;
    const uint8_t* frame = reinterpret_cast<const uint8_t*>(query.mReplyData);
    /* Video frame is always first. */
    if (vframe != NULL && vframe_size != 0) {
        /* Make sure that video frame is in. */
        if ((query.mReplyDataSize - cur_offset) >= vframe_size) {
            memcpy(vframe, frame, vframe_size);
            cur_offset += vframe_size;
        } else {
            ALOGE("%s: Reply %d bytes is to small to contain %d bytes video frame",
                 __FUNCTION__, query.mReplyDataSize - cur_offset, vframe_size);
            return EINVAL;
        }
    }
    if (pframe != NULL && pframe_size != 0) {
        /* Make sure that preview frame is in. */
        if ((query.mReplyDataSize - cur_offset) >= pframe_size) {
            memcpy(pframe, frame + cur_offset, pframe_size);
            cur_offset += pframe_size;
        } else {
            ALOGE("%s: Reply %d bytes is to small to contain %d bytes preview frame",
                 __FUNCTION__, query.mReplyDataSize - cur_offset, pframe_size);
            return EINVAL;
        }
    }

    return NO_ERROR;
}

}; /* namespace android */

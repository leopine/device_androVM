/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <cutils/log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include "nusensors.h"

typedef struct s_sensor_data {
    u_int64_t sensor;
    double x;
    double y;
    double z;
} t_sensor_data;

static int server_sock = -1;
static int client_sock = -1;

static int64_t timeout = 200000000;

static sensors_event_t **last_event = NULL;

static int poll__close(struct hw_device_t *dev)
{
    ALOGD("poll__close(%p);", dev);

    if (dev) {
        free(dev);
    }

    if (client_sock != -1) {
        close(client_sock);
    }

    if (server_sock != -1) {
        close(server_sock);
    }

    int i = 0;
    while (i < SENSOR_MAX) {
        if (last_event[i]) {
            free(last_event[i]);
        }
        ++i;
    }

    free(last_event);

    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev, int handle, int enabled)
{
    ALOGD("poll__activate(%p, %d, %d);", dev, handle, enabled);

    server_sock = socket_inaddr_any_server(22471, SOCK_STREAM);

    if (server_sock < 0) {
        ALOGE("Unable to start listening server: %d", errno);
        return -1;
    }

    last_event = (sensors_event_t **)malloc(sizeof(*last_event) * SENSOR_MAX);

    if (!last_event) {
        return -1;
    }

    int i = 0;
    while (i < SENSOR_MAX) {
        last_event[i] = NULL;
        ++i;
    }

    return 0;
}

static int poll__setDelay(struct sensors_poll_device_t *dev, int handle, int64_t ns)
{
    ALOGD("poll__setDelay(%p, %d, %lld);", dev, handle, ns);

    timeout = ns;

    return 0;
}

static int64_t getTimestamp(void) {
    struct timespec t;

    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);

    return int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

static void waitAndAcceptNewClient(void)
{
    fd_set readfs;
    struct timeval max_delay;

    FD_ZERO(&readfs);
    FD_SET(server_sock, &readfs);

    max_delay.tv_sec = 0;
    max_delay.tv_usec = 100000;

    int ret = select(server_sock + 1, &readfs, NULL, NULL, &max_delay);

    if (ret != -1 && FD_ISSET(server_sock, &readfs)) {
        client_sock = accept(server_sock, NULL, NULL);
    }
}

static int returnLastEvents(sensors_event_t *data, int count)
{
    int j; // Iterate over last events
    int total; // Total events

    j = 0;
    total = 0;

    while (total < count && j < SENSOR_MAX) {
        if (last_event[j]) {
            memcpy(&data[total], last_event[j], sizeof(*data));
            ++total;
        }
        ++j;
    }

    return total;
}

static int readDataFromClient(sensors_event_t *data, int count)
{
    fd_set readfs;
    struct timeval max_delay;

    FD_ZERO(&readfs);
    FD_SET(client_sock, &readfs);

    max_delay.tv_sec = 0;
    max_delay.tv_usec = (timeout / 1000) % 1000000;

    int ret = select(client_sock + 1, &readfs, NULL, NULL, &max_delay);

    if (ret != -1 && FD_ISSET(client_sock, &readfs)) {

        t_sensor_data adata[count];

        int sread = recv(client_sock, adata, sizeof(*adata) * count, 0);

        if (sread <= 0) {
            ALOGE("Error reading datas, errno=%d", errno);
            close(client_sock);
            client_sock = -1;
            return returnLastEvents(data, count);
        }

        if ((sread % sizeof(*adata)) != 0) {
            ALOGD("read() returned %d bytes, not a multiple of %d !", sread, sizeof(*adata));
            return returnLastEvents(data, count);
        }

        int nbEvents = sread / sizeof(*adata);

        for (int i = 0; i < nbEvents; i++) {
            if (adata[i].sensor != SENSOR_TYPE_ACCELEROMETER) {
                ALOGI("Unknown sensor type : %lld !", adata[i].sensor);
                continue;
            }

            if (!last_event[adata[i].sensor]) {
                last_event[adata[i].sensor] = (sensors_event_t *)malloc(sizeof(**last_event));
            }

            data[i].version = sizeof(sensors_event_t);
            data[i].sensor = 0;
            data[i].type = SENSOR_TYPE_ACCELEROMETER;
            data[i].timestamp = getTimestamp();
            memset(data[i].data, 0, sizeof(data[i].data));
            data[i].acceleration.x = adata[i].x;
            data[i].acceleration.y = adata[i].y;
            data[i].acceleration.z = adata[i].z;

            // Save this event as the last good event for this sensor
            if (last_event[adata[i].sensor]) {
                memcpy(last_event[adata[i].sensor], &data[i], sizeof(**last_event));
            }
        }

        return nbEvents;
    } else {
        return returnLastEvents(data, count);
    }
}

static int poll__poll(struct sensors_poll_device_t *dev, sensors_event_t *data, int count)
{
    ALOGD("poll__poll(%p, %p, %d);", dev, data, count);

    if (client_sock == -1) {
        waitAndAcceptNewClient();
        return returnLastEvents(data, count);
    } else {
        return readDataFromClient(data, count);
    }
}

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device)
{
    struct sensors_poll_device_t *dev;

    dev = (struct sensors_poll_device_t *)malloc(sizeof(struct sensors_poll_device_t));
    if (!dev) {
        ALOGE("Unable to allocate sensors_poll_device_t");
        return -1;
    }

    memset(dev, 0, sizeof(sensors_poll_device_t));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version  = 0;
    dev->common.module   = const_cast<hw_module_t*>(module);
    dev->common.close    = poll__close;
    dev->activate        = poll__activate;
    dev->setDelay        = poll__setDelay;
    dev->poll            = poll__poll;

    *device = &dev->common;
    return 0;
}

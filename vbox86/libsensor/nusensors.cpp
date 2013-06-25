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

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <strings.h>

#include <poll.h>
#include <pthread.h>

#include <linux/input.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include <hardware/sensors.h>

#include "nusensors.h"

struct vsensor_accel_data {
    unsigned char id;
    unsigned char res1;
    char val_x[16];
    char val_y[16];
    char val_z[16];
};


static int poll__close(struct hw_device_t *dev)
{
    if (dev)
        free(dev);
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    ALOGE("poll__activate() with handle=%d enabled=%d", handle, enabled);
    return 0;
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    ALOGE("poll__setDelay() with handle=%d ns=%ld", handle, ns);
    return 0;
}

static int64_t getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count) {
    static int csocket = -1;
    int n = 0;

    ALOGD("poll() called for %d events", count);

 socket_connect:
    while (csocket < 0) {
        int ssocket;

        sleep(1);

        ssocket = socket_inaddr_any_server(22471, SOCK_STREAM);

        if (ssocket < 0) {
            ALOGE("Unable to start listening server: %d", errno);
            continue;
        }

        csocket = accept(ssocket, NULL, NULL);

        if (csocket < 0) {
            ALOGE("Unable to accept connection: %d", errno);
            close(ssocket);
            continue;
        }

        close(ssocket);

        ALOGI("Sensor server connected");
    }

    // Get the data
    while (1) {

        sleep(1);

        t_sensor_data adata[count];

        ALOGD("Waiting for %d events", count);

        int sread = recv(csocket, adata, sizeof(*adata) * count, 0);

        if (sread < 0) {
            ALOGE("Error reading datas, errno=%d", errno);
            close(csocket);
            csocket = -1;
            goto socket_connect;
        }
        if (sread == 0) {
            ALOGE("connection closed, reconnecting...");
            close(csocket);
            csocket = -1;
            goto socket_connect;
        }
        if ((sread % sizeof(*adata)) != 0) {
            ALOGD("read() returned %d bytes, not a multiple of %d !", sread, sizeof(vsensor_accel_data));
            continue;
        }

        int nbEvents = sread / sizeof(*adata);


        for (int i=0; i<nbEvents; i++) {
            if (adata[i].sensor != SENSOR_TYPE_ACCELEROMETER) {
                ALOGI("Unknown sensor type : %d !", adata[i].sensor);
                continue;
            }

            data[i].version = sizeof(sensors_event_t);
            data[i].sensor = 0;
            data[i].type = SENSOR_TYPE_ACCELEROMETER;
            data[i].timestamp = getTimestamp();
            memset(data[i].data, 0, sizeof(data[i].data));
            data[i].acceleration.x = adata[i].x;
            data[i].acceleration.y = adata[i].y;
            data[i].acceleration.z = adata[i].z;
        }

        return nbEvents;
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

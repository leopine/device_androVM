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
 * Contains implementation of a class EmulatedCameraFactory that manages cameras
 * available for emulation.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_Factory"
#include <cutils/log.h>
#include <cutils/properties.h>
#include "EmulatedGenyCamera.h"
#include "EmulatedFakeCamera.h"
#include "EmulatedFakeCamera2.h"
#include "EmulatedCameraFactory.h"

extern camera_module_t HAL_MODULE_INFO_SYM;

/* A global instance of EmulatedCameraFactory is statically instantiated and
 * initialized when camera emulation HAL is loaded.
 */
android::EmulatedCameraFactory  gEmulatedCameraFactory;

namespace android {

EmulatedCameraFactory::EmulatedCameraFactory()
        : mEmulatedCameras(NULL),
          mEmulatedCameraNum(0),
          mFakeCameraNum(0),
          mConstructedOK(false)
{
    status_t res;

    /* Initialize our 2 fakes Geny camera (front and back) */
    /* TODO: we might need to read properties to enable or not cameras */
    createGenyCameras();

    /* If fake camera(s) is(are) activated add them to the list of camera */
    /* TODO: we might remove this code, because fake camera might be emulated by
       the player */
    if (isBackFakeCameraEmulationOn()) {
        /* Camera ID. */
        const int camera_id = mEmulatedCameraNum;
        /* Use fake camera to emulate back-facing camera. */
        mEmulatedCameraNum++;

        /* Make sure that array is allocated (in case there were no 'qemu'
         * cameras created. Note that we preallocate the array so it may contain
         * two fake cameras: one facing back, and another facing front. */
        if (mEmulatedCameras == NULL) {
            mEmulatedCameras = new EmulatedBaseCamera*[mEmulatedCameraNum + 1];
            if (mEmulatedCameras == NULL) {
                ALOGE("%s: Unable to allocate emulated camera array for %d entries",
                     __FUNCTION__, mEmulatedCameraNum);
                return;
            }
            memset(mEmulatedCameras, 0,
                    (mEmulatedCameraNum + 1) * sizeof(EmulatedBaseCamera*));
        }

        /* Create, and initialize the fake camera */
        switch (getBackCameraHalVersion()) {
            case 1:
                mEmulatedCameras[camera_id] =
                        new EmulatedFakeCamera(camera_id, true,
                                &HAL_MODULE_INFO_SYM.common);
                break;
            case 2:
                mEmulatedCameras[camera_id] =
                        new EmulatedFakeCamera2(camera_id, true,
                                &HAL_MODULE_INFO_SYM.common);
                break;
            default:
                ALOGE("%s: Unknown back camera hal version requested: %d", __FUNCTION__,
                        getBackCameraHalVersion());
        }
        if (mEmulatedCameras[camera_id] != NULL) {
            ALOGV("%s: Back camera device version is %d", __FUNCTION__,
                    getBackCameraHalVersion());
            res = mEmulatedCameras[camera_id]->Initialize();
            if (res != NO_ERROR) {
                ALOGE("%s: Unable to intialize back camera %d: %s (%d)",
                        __FUNCTION__, camera_id, strerror(-res), res);
                delete mEmulatedCameras[camera_id];
                mEmulatedCameraNum--;
            }
        } else {
            mEmulatedCameraNum--;
            ALOGE("%s: Unable to instantiate fake camera class", __FUNCTION__);
        }
    }

    if (isFrontFakeCameraEmulationOn()) {
        /* Camera ID. */
        const int camera_id = mEmulatedCameraNum;
        /* Use fake camera to emulate front-facing camera. */
        mEmulatedCameraNum++;

        /* Make sure that array is allocated (in case there were no 'qemu'
         * cameras created. */
        if (mEmulatedCameras == NULL) {
            mEmulatedCameras = new EmulatedBaseCamera*[mEmulatedCameraNum];
            if (mEmulatedCameras == NULL) {
                ALOGE("%s: Unable to allocate emulated camera array for %d entries",
                     __FUNCTION__, mEmulatedCameraNum);
                return;
            }
            memset(mEmulatedCameras, 0,
                    mEmulatedCameraNum * sizeof(EmulatedBaseCamera*));
        }

        /* Create, and initialize the fake camera */
        switch (getFrontCameraHalVersion()) {
            case 1:
                mEmulatedCameras[camera_id] =
                        new EmulatedFakeCamera(camera_id, false,
                                &HAL_MODULE_INFO_SYM.common);
                break;
            case 2:
                mEmulatedCameras[camera_id] =
                        new EmulatedFakeCamera2(camera_id, false,
                                &HAL_MODULE_INFO_SYM.common);
                break;
            default:
                ALOGE("%s: Unknown front camera hal version requested: %d",
                        __FUNCTION__,
                        getFrontCameraHalVersion());
        }
        if (mEmulatedCameras[camera_id] != NULL) {
            ALOGV("%s: Front camera device version is %d", __FUNCTION__,
                    getFrontCameraHalVersion());
            res = mEmulatedCameras[camera_id]->Initialize();
            if (res != NO_ERROR) {
                ALOGE("%s: Unable to intialize front camera %d: %s (%d)",
                        __FUNCTION__, camera_id, strerror(-res), res);
                delete mEmulatedCameras[camera_id];
                mEmulatedCameraNum--;
            }
        } else {
            mEmulatedCameraNum--;
            ALOGE("%s: Unable to instantiate fake camera class", __FUNCTION__);
        }
    }

    ALOGV("%d cameras are being emulated. %d of them are fake cameras.",
          mEmulatedCameraNum, mFakeCameraNum);

    mConstructedOK = true;
}

EmulatedCameraFactory::~EmulatedCameraFactory()
{
    if (mEmulatedCameras != NULL) {
        for (int n = 0; n < mEmulatedCameraNum; n++) {
            if (mEmulatedCameras[n] != NULL) {
                delete mEmulatedCameras[n];
            }
        }
        delete[] mEmulatedCameras;
    }
}

/****************************************************************************
 * Camera HAL API handlers.
 *
 * Each handler simply verifies existence of an appropriate EmulatedBaseCamera
 * instance, and dispatches the call to that instance.
 *
 ***************************************************************************/

int EmulatedCameraFactory::cameraDeviceOpen(int camera_id, hw_device_t** device)
{
    ALOGV("%s: id = %d", __FUNCTION__, camera_id);

    *device = NULL;

    if (!isConstructedOK()) {
        ALOGE("%s: EmulatedCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getEmulatedCameraNum()) {
        ALOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getEmulatedCameraNum());
        return -EINVAL;
    }

    return mEmulatedCameras[camera_id]->connectCamera(device);
}

int EmulatedCameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    ALOGV("%s: id = %d", __FUNCTION__, camera_id);

    if (!isConstructedOK()) {
        ALOGE("%s: EmulatedCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getEmulatedCameraNum()) {
        ALOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getEmulatedCameraNum());
        return -EINVAL;
    }

    return mEmulatedCameras[camera_id]->getCameraInfo(info);
}

/****************************************************************************
 * Camera HAL API callbacks.
 ***************************************************************************/

int EmulatedCameraFactory::device_open(const hw_module_t* module,
                                       const char* name,
                                       hw_device_t** device)
{
    /*
     * Simply verify the parameters, and dispatch the call inside the
     * EmulatedCameraFactory instance.
     */

    if (module != &HAL_MODULE_INFO_SYM.common) {
        ALOGE("%s: Invalid module %p expected %p",
             __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }
    if (name == NULL) {
        ALOGE("%s: NULL name is not expected here", __FUNCTION__);
        return -EINVAL;
    }

    return gEmulatedCameraFactory.cameraDeviceOpen(atoi(name), device);
}

int EmulatedCameraFactory::get_number_of_cameras(void)
{
    return gEmulatedCameraFactory.getEmulatedCameraNum();
}

int EmulatedCameraFactory::get_camera_info(int camera_id,
                                           struct camera_info* info)
{
    return gEmulatedCameraFactory.getCameraInfo(camera_id, info);
}

/********************************************************************************
 * Internal API
 *******************************************************************************/

/*
 * Cameras initialization
 */

/* TODO: move this in the DeviceCamera */
static const char lListNameToken[]    = "name=";
/* Frame dimensions token. */
static const char lListDimsToken[]    = "framedims=";
/* Facing direction token. */
static const char lListDirToken[]     = "dir=";
/* /TODO */

void EmulatedCameraFactory::createGenyCameras()
{
    /* We have 2 cameras daemon: one for front and the other for back camera */
    /* this might change some day, so keep it dynamic */
    int num = 2;
    status_t res;

    /* Allocate the array for emulated camera instances. Note that we allocate
     * two more entries for back and front fake camera emulation. */
    mEmulatedCameras = new EmulatedBaseCamera*[num];
    if (mEmulatedCameras == NULL) {
        ALOGE("%s: Unable to allocate emulated camera array for %d entries",
             __FUNCTION__, num + 1);
        return;
    }
    memset(mEmulatedCameras, 0, sizeof(EmulatedBaseCamera*) * (num + 1));

    int index = 0;
    /* Create and initialize back camera. */
    EmulatedGenyCamera* geny_cam =
        new EmulatedGenyCamera(index, &HAL_MODULE_INFO_SYM.common);
    if (NULL != geny_cam) {
        res = geny_cam->Initialize("back", 24800/*DEFAULT_BACK_CAMERA_PORT*/);
        if (res == NO_ERROR) {
            mEmulatedCameras[index] = geny_cam;
            index++;
        } else {
            delete geny_cam;
        }
    } else {
        ALOGE("%s: Unable to instantiate Back Camera",
              __FUNCTION__);
    }

    /* Create and initialize front camera. */
    geny_cam = new EmulatedGenyCamera(index, &HAL_MODULE_INFO_SYM.common);
    if (NULL != geny_cam) {
        res = geny_cam->Initialize("front", 24810/*DEFAULT_FRONT_CAMERA_PORT*/);
        if (res == NO_ERROR) {
            mEmulatedCameras[index] = geny_cam;
            index++;
        } else {
            delete geny_cam;
        }
    } else {
        ALOGE("%s: Unable to instantiate Front Camera",
              __FUNCTION__);
    }

    mEmulatedCameraNum = index;
}

bool EmulatedCameraFactory::isBackFakeCameraEmulationOn()
{
    /* Defined by 'qemu.sf.fake_camera' boot property: if property exist, and
     * is set to 'both', or 'back', then fake camera is used to emulate back
     * camera. */
    char prop[PROPERTY_VALUE_MAX];
    if ((property_get("qemu.sf.fake_camera", prop, NULL) > 0) &&
        (!strcmp(prop, "both") || !strcmp(prop, "back"))) {
        return true;
    } else {
        return false;
    }
}

int EmulatedCameraFactory::getBackCameraHalVersion()
{
    /* Defined by 'qemu.sf.back_camera_hal_version' boot property: if the
     * property doesn't exist, it is assumed to be 1. */
    char prop[PROPERTY_VALUE_MAX];
    if (property_get("qemu.sf.back_camera_hal", prop, NULL) > 0) {
        char *prop_end = prop;
        int val = strtol(prop, &prop_end, 10);
        if (*prop_end == '\0') {
            return val;
        }
        // Badly formatted property, should just be a number
        ALOGE("qemu.sf.back_camera_hal is not a number: %s", prop);
    }
    return 1;
}

bool EmulatedCameraFactory::isFrontFakeCameraEmulationOn()
{
    /* Defined by 'qemu.sf.fake_camera' boot property: if property exist, and
     * is set to 'both', or 'front', then fake camera is used to emulate front
     * camera. */
    char prop[PROPERTY_VALUE_MAX];
    if ((property_get("qemu.sf.fake_camera", prop, NULL) > 0) &&
        (!strcmp(prop, "both") || !strcmp(prop, "front"))) {
        return true;
    } else {
        return false;
    }
}

int EmulatedCameraFactory::getFrontCameraHalVersion()
{
    /* Defined by 'qemu.sf.front_camera_hal_version' boot property: if the
     * property doesn't exist, it is assumed to be 1. */
    char prop[PROPERTY_VALUE_MAX];
    if (property_get("qemu.sf.front_camera_hal", prop, NULL) > 0) {
        char *prop_end = prop;
        int val = strtol(prop, &prop_end, 10);
        if (*prop_end == '\0') {
            return val;
        }
        // Badly formatted property, should just be a number
        ALOGE("qemu.sf.front_camera_hal is not a number: %s", prop);
    }
    return 1;
}

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t EmulatedCameraFactory::mCameraModuleMethods = {
    open: EmulatedCameraFactory::device_open
};

}; /* namespace android */

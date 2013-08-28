/*
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
 * Contains implementation of a class EmulatedGenyCamera that encapsulates
 * functionality of an emulated camera connected to the host.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_GenyCamera"
#include <cutils/log.h>
#include <cutils/properties.h>
#include "EmulatedGenyCamera.h"
#include "EmulatedCameraFactory.h"

#define NAME_TOKEN "name="
#define DIR_TOKEN  "dir="
#define DIM_TOKEN  "framedims="

namespace android {

EmulatedGenyCamera::EmulatedGenyCamera(int cameraId, struct hw_module_t* module)
        : EmulatedCamera(cameraId, module),
          mGenyCameraDevice(this)
{
}

EmulatedGenyCamera::~EmulatedGenyCamera()
{
}

/****************************************************************************
 * EmulatedCamera virtual overrides.
 ***************************************************************************/

status_t EmulatedGenyCamera::Initialize(const char* device_name, const int local_srv_port)
{
    ALOGV("%s:\n   initializing %s camera on local srv port %d",
          __FUNCTION__, device_name, local_srv_port);

    /* Initialize camera device. */
    status_t res = mGenyCameraDevice.Initialize(device_name, local_srv_port);
    if (res != NO_ERROR) {
        return res;
    }

    /*
     * Initialize base class.
     */
    res = EmulatedCamera::Initialize();
    if (res != NO_ERROR) {
        return res;
    }

    /*
     * Get information from the webcam device connected to host
     */
    char *info_str = NULL;
    res = mGenyCameraDevice.getDeviceInfo(&info_str);
    if (res != NO_ERROR) {
        ALOGE("%s: Failed to get device information for %s",
              __FUNCTION__, device_name);
        return res;
    }

    /* Parse information from the returned payload */
    ALOGV("%s: Camera info: '%s'", __FUNCTION__, info_str);
    /* Find 'framedims', and 'dir' tokens that are required here. */
    char* dim_start = strstr(info_str, DIM_TOKEN);
    char* dir_start = strstr(info_str, DIR_TOKEN);
    if (dim_start != NULL && dir_start != NULL) {
        /* Advance to the token values. */
        dim_start += strlen(DIM_TOKEN);
        dir_start += strlen(DIR_TOKEN);

        /* Terminate token values with zero. */
        char* s = strchr(dim_start, ' ');
        if (s != NULL) {
            *s = '\0';
        }
        s = strchr(dir_start, ' ');
        if (s != NULL) {
            *s = '\0';
        }
    } else {
        ALOGE("%s: Failed to parse camera info", __FUNCTION__);
        free(info_str);
        return EINVAL;
    }

    /*
     * Set customizable parameters that overload EmulatedCamera ones
     */
    mParameters.set(EmulatedCamera::FACING_KEY,
                    (strcmp(dir_start, EmulatedCamera::FACING_FRONT) == 0) ?
                    EmulatedCamera::FACING_FRONT :
                    EmulatedCamera::FACING_BACK);
    mParameters.set(EmulatedCamera::ORIENTATION_KEY,
                    gEmulatedCameraFactory.getQemuCameraOrientation());
    mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, dim_start);
    mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, dim_start);

    int x, y;
    if (parseXYFromList(dim_start, x, y) != NO_ERROR) {
        x = 320;
        y = 240;
        ALOGE("%s: failed to parse first resolution, defaulting to %dx%d",
              __FUNCTION__, x, y);
    }
    mParameters.setPreviewSize(x, y);
    mParameters.setPictureSize(x, y);

    ALOGV("%s: Geny camera %s is initialized. Current frame is %dx%d",
          __FUNCTION__, device_name, x, y);

    free(info_str);
    return NO_ERROR;
}

EmulatedCameraDevice* EmulatedGenyCamera::getCameraDevice()
{
    return &mGenyCameraDevice;
}

status_t EmulatedGenyCamera::startPreview()
{
    /* Overload parameters if needed */
    char prop[PROPERTY_VALUE_MAX];
    if ((property_get("genyd.camera.preview.dims", prop, NULL) > 0)) {
        int x, y;
        if (parseXYFromList(prop, x, y) == NO_ERROR) {
            /* Override Initialization configuration */
            mParameters.setPreviewSize(x, y);
            mParameters.setPictureSize(x, y);
            ALOGV("%s: Camera overload properties with %dx%d resolution",
                  __FUNCTION__, x, y);

        } else {
            ALOGE("%s: Wrong resolution format specified in prop: %s",
                  __FUNCTION__, prop);
        }
    }

    /* Call parent method */
    return EmulatedCamera::startPreview();
}

/* Function useful for debugging */
status_t EmulatedGenyCamera::setParameters(const char *parms)
{
    /* Overload parent EmulateCamera setParameters */
    ALOGV("%s: overriding parent method, params: %s",
          __FUNCTION__, parms);
    return EmulatedCamera::setParameters(parms);
}

status_t EmulatedGenyCamera::parseXYFromList(const char *dims_list, int &x, int &y)
{
    /*
     * Parse first usable resolution in the input list (..x..,..x..,etc.)
     */
    char first_dim[128];
    /* Dimensions are separated with ',' */
    const char* c = strchr(dims_list, ',');
    if (c != NULL) {
        snprintf(first_dim, sizeof(first_dim), "%.*s", c - dims_list, dims_list);
    } else {
        snprintf(first_dim, sizeof(first_dim), "%s", dims_list);
    }

    /* Width and height are separated with 'x' */
    char* sep = strchr(first_dim, 'x');
    if (sep == NULL) {
        ALOGE("%s: Invalid first dimension format in %s",
             __FUNCTION__, dims_list);
        return EINVAL;
    }

    *sep = '\0';
    x = atoi(first_dim);
    y = atoi(sep + 1);

    return NO_ERROR;
}

};  /* namespace android */

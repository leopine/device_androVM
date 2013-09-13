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
    // hardcode camera capabilities
    const char dim_list[] = "640x480,352x288,320x240,176x144";
    int x = 640;
    int y = 480;
    /*
     * Set customizable parameters that overload EmulatedCamera ones
     */

    mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
                    "30,25,24,20,15,10,5");

    mParameters.set(EmulatedCamera::FACING_KEY,
                    (strcmp(device_name, EmulatedCamera::FACING_FRONT) == 0) ?
                    EmulatedCamera::FACING_FRONT :
                    EmulatedCamera::FACING_BACK);
    mParameters.set(EmulatedCamera::ORIENTATION_KEY, getCameraOrientation());
    mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, dim_list);
    mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, dim_list);
    mParameters.setPreviewSize(x, y);
    mParameters.setPictureSize(x, y);

    ALOGV("%s: Geny camera %s is initialized. Current frame is %dx%d",
          __FUNCTION__, device_name, x, y);

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

int EmulatedGenyCamera::getCameraOrientation()
{
    // Set Camera orientation according to device resolution:
    // If in landscape resolution, camera should have an orientation of 0°
    // and an orientation of 270° in portrait resolution
    // see hardware/libhardware/include/camera/camera_common.h
    char prop[PROPERTY_VALUE_MAX];
    int height, width, depth, orientation = 0;

    if ((property_get("androVM.vbox_graph_mode", prop, NULL) > 0) &&
        sscanf(prop, "%dx%d-%d", &width, &height, &depth) == 3) {
        // If portrait resolution change camera orientation
        if (height > width) {
            orientation = 270;
        }
    }
    ALOGV("%s: camera orientation set to %d", __FUNCTION__, orientation);
    return orientation;
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

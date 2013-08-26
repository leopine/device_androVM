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
#include "EmulatedGenyCamera.h"
#include "EmulatedCameraFactory.h"

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

    /* Initialize base class. */
    res = EmulatedCamera::Initialize();
    if (res != NO_ERROR) {
        return res;
    }

    /*
     * Set customizable parameters.
     */
    /*
    ALOGV("%s: Geny camera %s is initialized. Current frame is %dx%d",
         __FUNCTION__, device_name, x, y);
    */
    return NO_ERROR;
}

EmulatedCameraDevice* EmulatedGenyCamera::getCameraDevice()
{
    return &mGenyCameraDevice;
}

};  /* namespace android */

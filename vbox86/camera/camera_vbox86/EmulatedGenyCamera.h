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

#ifndef HW_EMULATOR_CAMERA_EMULATED_GENY_CAMERA_H
#define HW_EMULATOR_CAMERA_EMULATED_GENY_CAMERA_H

/*
 * Contains declaration of a class EmulatedGenyCamera that encapsulates
 * functionality of an emulated camera connected to the host.
 */

#include "EmulatedCamera.h"
#include "EmulatedGenyCameraDevice.h"

namespace android {

/* Encapsulates functionality of an emulated camera connected to the host.
 */
class EmulatedGenyCamera : public EmulatedCamera {
public:
    /* Constructs EmulatedGenyCamera instance. */
    EmulatedGenyCamera(int cameraId, struct hw_module_t* module);

    /* Destructs EmulatedGenyCamera instance. */
    ~EmulatedGenyCamera();

    /***************************************************************************
     * EmulatedCamera virtual overrides.
     **************************************************************************/

public:
    /* Initializes EmulatedGenyCamera instance. */
    status_t Initialize(const char* device_name, const int local_srv_port);
    /* Override parent to alter preview parameters if needed */
    status_t startPreview(void);
    status_t setParameters(const char *parms);

    /***************************************************************************
     * EmulatedCamera abstract API implementation.
     **************************************************************************/

protected:
    /* Gets emulated camera device used by this instance of the emulated camera.
     */
    EmulatedCameraDevice* getCameraDevice();

    /* Tool function to parse resolution X Y from a resolution list. */
    status_t parseXYFromList(const char *dims_list, int &x, int &y);

    /***************************************************************************
     * Data members.
     **************************************************************************/

protected:
    /* Contained geny camera device object. */
    EmulatedGenyCameraDevice    mGenyCameraDevice;
};

}; /* namespace android */

#endif  /* HW_EMULATOR_CAMERA_EMULATED_GENY_CAMERA_H */

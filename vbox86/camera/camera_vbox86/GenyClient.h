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

#ifndef HW_EMULATOR_CAMERA_GENY_CLIENT_H
#define HW_EMULATOR_CAMERA_GENY_CLIENT_H

#include "QemuClient.h"

namespace android {

/****************************************************************************
 * Geny client base
 ***************************************************************************/

/*
 * Contains declaration of classes that encapsulate connection to camera services
 * in the VM via local_camera daemon
 */

class GenyClient {
public:
    /* Constructs GenyClient instance. */
    GenyClient();

    /* Destructs QemuClient instance. */
    virtual ~GenyClient();

    /****************************************************************************
     * Geny client API
     ***************************************************************************/

public:
    /* TODO */
    /* Connects to the 'camera' service in the emulator via qemu pipe.
     * Param:
     *  param - Parameters to pass to the camera service. There are two types of
     *      camera services implemented by the emulator. The first one is a
     *      'camera factory' type of service that provides list of cameras
     *      connected to the host. Another one is an 'emulated camera' type of
     *      service that provides interface to a camera connected to the host. At
     *      the connection time emulator makes distinction between the two by
     *      looking at connection parameters: no parameters means connection to
     *      the 'factory' service, while connection with parameters means
     *      connection to an 'emulated camera' service, where camera is identified
     *      by one of the connection parameters. So, passing NULL, or an empty
     *      string to this method will establish a connection with the 'factory'
     *      service, while not empty string passed here will establish connection
     *      with an 'emulated camera' service. Parameters defining the emulated
     *      camera must be formatted as such:
     *
     *          "name=<device name> [inp_channel=<input channel #>]",
     *
     *      where 'device name' is a required parameter defining name of the
     *      camera device, and 'input channel' is an optional parameter (positive
     *      integer), defining the input channel to use on the camera device.
     *      Note that device name passed here must have been previously obtained
     *      from the factory service using 'list' query.
     * Return:
     *  NO_ERROR on success, or an appropriate error status.
     */
    virtual status_t connectClient(const int local_srv_port);

    /* Disconnects from the service. */
    virtual void disconnectClient();

    /* Sends data to the service.
     * Param:
     *  data, data_size - Data to send.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    virtual status_t sendMessage(const void* data, size_t data_size);

    /* Receives data from the service.
     * This method assumes that data to receive will come in two chunks: 8
     * characters encoding the payload size in hexadecimal string, followed by
     * the paylod (if any).
     * This method will allocate data buffer where to receive the response.
     * Param:
     *  data - Upon success contains address of the allocated data buffer with
     *      the data received from the service. The caller is responsible for
     *      freeing allocated data buffer.
     *  data_size - Upon success contains size of the data received from the
     *      service.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    virtual status_t receiveMessage(void** data, size_t* data_size);

    /* Sends a query, and receives a response from the service.
     * Param:
     *  query - Query to send to the service. When this method returns, the query
     *  is completed, and all its relevant data members are properly initialized.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure. Note that
     *  status returned here is not the final query status. Use isQuerySucceeded(),
     *  or getCompletionStatus() method on the query object to see if it has
     *  succeeded. However, if this method returns a failure, it means that the
     *  query has failed, and there is no guarantee that its data members are
     *  properly initialized (except for the 'mQueryDeliveryStatus', which is
     *  always in the proper state).
     */
    virtual status_t doQuery(QemuQuery* query);

    /****************************************************************************
     * Data members
     ***************************************************************************/

protected:
    /* local_camera socket handle. */
    int     mSocketFD;

};

/****************************************************************************
 * Qemu client for an 'emulated camera' service.
 ***************************************************************************/

/* Encapsulates QemuClient for an 'emulated camera' service.
 */
class CameraGenyClient : public GenyClient {
public:
    /* Constructs CameraQemuClient instance. */
    CameraGenyClient();

    /* Destructs CameraGenyClient instance. */
    ~CameraGenyClient();

    /****************************************************************************
     * Public API
     ***************************************************************************/

public:
    /* Queries camera connection.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    status_t queryConnect();

    /* Queries camera disconnection.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    status_t queryDisconnect();

    /* Queries camera info.
     * Param:
     *  p_info_string - return an allocated string containing device info
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    status_t queryInfo(char **p_info_string);

    /* Queries camera to start capturing video.
     * Param:
     *  pixel_format - Pixel format that is used by the client to push video
     *      frames to the camera framework.
     *  width, height - Frame dimensions, requested by the framework.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    status_t queryStart(uint32_t pixel_format, int width, int height);

    /* Queries camera to stop capturing video.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    status_t queryStop();

    /* Queries camera for the next video frame.
     * Param:
     *  vframe, vframe_size - Define buffer, allocated to receive a video frame.
     *      Any of these parameters can be 0, indicating that the caller is
     *      interested only in preview frame.
     *  pframe, pframe_size - Define buffer, allocated to receive a preview frame.
     *      Any of these parameters can be 0, indicating that the caller is
     *      interested only in video frame.
     *  r_scale, g_scale, b_scale - White balance scale.
     *  exposure_comp - Expsoure compensation.
     * Return:
     *  NO_ERROR on success, or an appropriate error status on failure.
     */
    status_t queryFrame(void* vframe,
                        void* pframe,
                        size_t vframe_size,
                        size_t pframe_size,
                        float r_scale,
                        float g_scale,
                        float b_scale,
                        float exposure_comp);

    /****************************************************************************
     * Names of the queries available for the emulated camera.
     ***************************************************************************/

private:
    /* Connect to the camera. */
    static const char mQueryConnect[];
    /* Disconnect from the camera. */
    static const char mQueryDisconnect[];
    /* Query info from the webcam */
    static const char mQueryInfo[];
    /* Start video capturing. */
    static const char mQueryStart[];
    /* Stop video capturing. */
    static const char mQueryStop[];
    /* Query frame(s). */
    static const char mQueryFrame[];
};

}; /* namespace android */

#endif  /* HW_EMULATOR_CAMERA_GENY_CLIENT_H */

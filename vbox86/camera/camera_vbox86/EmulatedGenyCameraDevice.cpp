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
 * Contains implementation of a class EmulatedGenyCameraDevice that encapsulates
 * an emulated camera device connected to the host.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_GenyDevice"
#include <cutils/log.h>
#include "EmulatedGenyCamera.h"
#include "EmulatedGenyCameraDevice.h"

#define DEBUG_LATENCY 0
#if DEBUG_LATENCY
#define LOG_LAT(...) ALOGD(__VA_ARGS__)
#define IFDEBUG_TIME_MS (systemTime() / 1000000L)
#else
#define LOG_LAT(...) (void(0))
#define IFDEBUG_TIME_MS 0
#endif /* DEBUG_LATENCY */

namespace android {

EmulatedGenyCameraDevice::EmulatedGenyCameraDevice(EmulatedGenyCamera* camera_hal)
    : EmulatedCameraDevice(camera_hal),
      mGenyClient(),
      mLastFrame(0)
{
}

EmulatedGenyCameraDevice::~EmulatedGenyCameraDevice()
{
}

/****************************************************************************
 * Public API
 ***************************************************************************/

status_t EmulatedGenyCameraDevice::Initialize(const char* device_name, const int local_srv_port)
{
    /* Connect to the local_camera daemon. */

    status_t res = mGenyClient.connectClient(local_srv_port);
    if (res != NO_ERROR) {
        return res;
    }

    /* Initialize base class. */
    res = EmulatedCameraDevice::Initialize();
    if (res == NO_ERROR) {
        ALOGV("%s: Connected to the emulated camera service '%s'",
             __FUNCTION__, device_name);
        mDeviceName = device_name;
    } else {
        mGenyClient.queryDisconnect();
    }

    return res;
}


status_t EmulatedGenyCameraDevice::getDeviceInfo(char **p_info_str)
{
    ALOGV("%s", __FUNCTION__);
    Mutex::Autolock locker(&mObjectLock);
    if (!isInitialized()) {
        ALOGE("%s: Geny camera device is not initialized.", __FUNCTION__);
        return EINVAL;
    }

    status_t res = mGenyClient.queryInfo(p_info_str);
    if (res == NO_ERROR) {
        ALOGV("%s: Geny camera device infos :'%s'",
             __FUNCTION__, *p_info_str);
    } else {
        ALOGE("%s: Unable to get device info '%s' (%d)",
              __FUNCTION__, (const char*)mDeviceName), res;
    }

    return res;

}


/****************************************************************************
 * Emulated camera device abstract interface implementation.
 ***************************************************************************/

status_t EmulatedGenyCameraDevice::connectDevice()
{
    ALOGV("%s", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    if (!isInitialized()) {
        ALOGE("%s: Geny camera device is not initialized.", __FUNCTION__);
        return EINVAL;
    }
    if (isConnected()) {
        ALOGW("%s: Geny camera device '%s' is already connected.",
             __FUNCTION__, (const char*)mDeviceName);
        return NO_ERROR;
    }

    /* Connect to the camera device via emulator. */
    const status_t res = mGenyClient.queryConnect();
    if (res == NO_ERROR) {
        ALOGV("%s: Connected to device '%s'",
             __FUNCTION__, (const char*)mDeviceName);
        mState = ECDS_CONNECTED;
    } else {
        ALOGE("%s: Connection to device '%s' failed",
             __FUNCTION__, (const char*)mDeviceName);
    }

    return res;
}

status_t EmulatedGenyCameraDevice::disconnectDevice()
{
    ALOGV("%s", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    if (!isConnected()) {
        ALOGW("%s: Geny camera device '%s' is already disconnected.",
             __FUNCTION__, (const char*)mDeviceName);
        return NO_ERROR;
    }
    if (isStarted()) {
        ALOGE("%s: Cannot disconnect from the started device '%s.",
             __FUNCTION__, (const char*)mDeviceName);
        return EINVAL;
    }

    /* Disconnect from the camera device via emulator. */
    const status_t res = mGenyClient.queryDisconnect();
    if (res == NO_ERROR) {
        ALOGV("%s: Disonnected from device '%s'",
             __FUNCTION__, (const char*)mDeviceName);
        mState = ECDS_INITIALIZED;
    } else {
        ALOGE("%s: Disconnection from device '%s' failed",
             __FUNCTION__, (const char*)mDeviceName);
    }

    return res;
}

status_t EmulatedGenyCameraDevice::startDevice(int width,
                                               int height,
                                               uint32_t pix_fmt)
{
    ALOGV("%s", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    if (!isConnected()) {
        ALOGE("%s: Geny camera device '%s' is not connected.",
             __FUNCTION__, (const char*)mDeviceName);
        return EINVAL;
    }
    if (isStarted()) {
        ALOGW("%s: Geny camera device '%s' is already started.",
             __FUNCTION__, (const char*)mDeviceName);
        return NO_ERROR;
    }

    status_t res = EmulatedCameraDevice::commonStartDevice(width, height, pix_fmt);
    if (res != NO_ERROR) {
        ALOGE("%s: commonStartDevice failed", __FUNCTION__);
        return res;
    }

    /* Start the actual camera device. */
    res = mGenyClient.queryStart(mPixelFormat, mFrameWidth, mFrameHeight);
    if (res == NO_ERROR) {
        ALOGV("%s: Geny camera device '%s' is started for %.4s[%dx%d] frames",
             __FUNCTION__, (const char*)mDeviceName,
             reinterpret_cast<const char*>(&mPixelFormat),
             mFrameWidth, mFrameHeight);
        mState = ECDS_STARTED;
    } else {
        ALOGE("%s: Unable to start device '%s' for %.4s[%dx%d] frames",
             __FUNCTION__, (const char*)mDeviceName,
             reinterpret_cast<const char*>(&pix_fmt), width, height);
    }

    return res;
}

status_t EmulatedGenyCameraDevice::stopDevice()
{
    ALOGV("%s", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    if (!isStarted()) {
        ALOGW("%s: Geny camera device '%s' is not started.",
             __FUNCTION__, (const char*)mDeviceName);
        return NO_ERROR;
    }

    /* Stop the actual camera device. */
    status_t res = mGenyClient.queryStop();
    if (res == NO_ERROR) {
        EmulatedCameraDevice::commonStopDevice();
        mState = ECDS_CONNECTED;
        ALOGV("%s: Geny camera device '%s' is stopped",
             __FUNCTION__, (const char*)mDeviceName);
    } else {
        ALOGE("%s: Unable to stop device '%s'",
             __FUNCTION__, (const char*)mDeviceName);
    }

    return res;
}


/****************************************************************************
 * EmulatedCameraDevice virtual overrides
 ***************************************************************************/

status_t EmulatedGenyCameraDevice::getCurrentPreviewFrame(void* buffer)
{
    return EmulatedCameraDevice::getCurrentPreviewFrame(buffer);
}

/****************************************************************************
 * Worker thread management overrides.
 ***************************************************************************/

bool EmulatedGenyCameraDevice::inWorkerThread()
{
    int32_t now_msec;

    now_msec = systemTime() / 1000000L;
    LOG_LAT("%s: Asking a frame at %d", __FUNCTION__, now_msec);
    /* Wait till FPS timeout expires, or thread exit message is received. */
    int32_t wait_usec = ((1000000 / mEmulatedFPS) <= ((now_msec - mLastFrame) * 1000)  ?
                       1 /* don't wait (0 == NULL == endless wait)*/:
                      ((1000000 / mEmulatedFPS) - ((now_msec - mLastFrame) * 1000))
                      );
    LOG_LAT("%s: Will wait %d ms", __FUNCTION__, wait_usec);

    WorkerThread::SelectRes res =
        getWorkerThread()->Select(-1, wait_usec);
    if (res == WorkerThread::EXIT_THREAD) {
        ALOGV("%s: Worker thread has been terminated.", __FUNCTION__);
        return false;
    }

    /* Remember when we decided to take this frame */
    mLastFrame = systemTime() / 1000000L;
    LOG_LAT("%s: Ask for the frame at %d", __FUNCTION__, mLastFrame);
    /* Query frames from the service. */
    status_t query_res = mGenyClient.queryFrame(mCurrentFrame, NULL,
                                                 mFrameBufferSize,
                                                 0,
                                                 mWhiteBalanceScale[0],
                                                 mWhiteBalanceScale[1],
                                                 mWhiteBalanceScale[2],
                                                 mExposureCompensation);

    now_msec = IFDEBUG_TIME_MS;
    LOG_LAT("%s: Reply received at %d", __FUNCTION__, now_msec);

    if (query_res == NO_ERROR) {
        /* Timestamp the current frame, and notify the camera HAL. */
        mCurFrameTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);

        now_msec = IFDEBUG_TIME_MS;
        LOG_LAT("%s: Notify HAL at %d", __FUNCTION__, now_msec);

        mCameraHAL->onNextFrameAvailable(mCurrentFrame, mCurFrameTimestamp, this);

        now_msec = IFDEBUG_TIME_MS;
        LOG_LAT("%s: Finished at %d , (total:%d ms)", __FUNCTION__,
              now_msec,
              now_msec - mLastFrame);
        return true;
    } else {
        ALOGE("%s: Unable to get current video frame: %s",
             __FUNCTION__, strerror(query_res));
        mCameraHAL->onCameraDeviceError(CAMERA_ERROR_SERVER_DIED);
        return false;
    }

}

}; /* namespace android */

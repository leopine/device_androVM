LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := main.c server.c
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_CFLAGS:=-O2 -g
LOCAL_MODULE := local_camera

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

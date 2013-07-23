LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= vinput.c

LOCAL_CFLAGS:=-O2 -g 

LOCAL_MODULE:=vinput
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= vinput_seamless.c

LOCAL_CFLAGS:=-O2 -g 

LOCAL_MODULE:=vinput_seamless
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


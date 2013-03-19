LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                                      \
                  main.cpp                             \

LOCAL_MODULE:= genyd

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
                    bionic \

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -Werror=format

LOCAL_SHARED_LIBRARIES := libsysutils libcutils libnetutils liblog

include $(BUILD_EXECUTABLE)

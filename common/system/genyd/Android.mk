##################
## Genyd daemon ##
##################

# Build executable
LOCAL_PATH		:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES		:= main.cpp				\
			   genyd.cpp				\
			   socket.cpp           		\
			   dispatcher.cpp			\
		           $(call all-proto-files-under, .)

LOCAL_MODULE		:= genyd

LOCAL_C_INCLUDES	:= $(KERNEL_HEADERS)       		\
			   bionic                  		\
			   external/stlport/stlport		\
			   external/protobuf/src

LOCAL_MODULE_TAGS	:= optional

LOCAL_CFLAGS		:= -Werror=format

LOCAL_SHARED_LIBRARIES	:= libcutils libnetutils liblog libstlport

LOCAL_PROTOC_OPTIMIZE_TYPE := full

include $(BUILD_EXECUTABLE)


###################
## Genyd library ##
###################

# Build Library
include $(CLEAR_VARS)

LOCAL_CFLAGS            := -Wall

LOCAL_C_INCLUDES        := $(LOCAL_PATH) \
                           bionic \
                           external/stlport/stlport

LOCAL_SRC_FILES         := genymotion.cpp

LOCAL_MODULE_TAGS       := optional
LOCAL_SHARED_LIBRARIES  := liblog libcutils libstlport

LOCAL_MODULE            := libgenyd

include $(BUILD_SHARED_LIBRARY)

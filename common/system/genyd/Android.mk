LOCAL_PATH		:= $(call my-dir)

###################
## Genyd library ##
###################

# Build Library
include $(CLEAR_VARS)

LOCAL_CFLAGS            := -Wall

LOCAL_C_INCLUDES        := $(LOCAL_PATH) \
                           bionic \
                           external/stlport/stlport

LOCAL_SRC_FILES         := lib/libgenyd.cpp \
                           lib/sensor_battery.cpp

LOCAL_MODULE_TAGS       := optional
LOCAL_SHARED_LIBRARIES  := liblog libcutils libstlport

LOCAL_MODULE            := libgenyd

include $(BUILD_SHARED_LIBRARY)


##################
## Genyd daemon ##
##################

# Build executable
include $(CLEAR_VARS)

LOCAL_SRC_FILES		:= main.cpp				\
			   genyd.cpp				\
			   socket.cpp           		\
			   dispatcher.cpp			\
			   gps_handler.cpp                      \
			   battery_handler.cpp			\
			   capabilities_handler.cpp		\
			   accelerometer_handler.cpp		\
		           $(call all-proto-files-under, .)

LOCAL_MODULE		:= genyd

LOCAL_C_INCLUDES	:= $(KERNEL_HEADERS)       		  \
			   bionic                  		  \
			   external/stlport/stlport		  \
			   external/protobuf/src		  \
			   device/androVM/common/system/genyd/lib \
			   device/androVM/vbox86/libsensor

LOCAL_MODULE_TAGS	:= optional

LOCAL_CFLAGS		:= -Werror=format

LOCAL_SHARED_LIBRARIES	:= libcutils	\
			   libnetutils	\
			   liblog	\
			   libstlport	\
			   libgenyd

LOCAL_PROTOC_OPTIMIZE_TYPE := full

include $(BUILD_EXECUTABLE)

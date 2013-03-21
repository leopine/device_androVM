LOCAL_PATH		:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES		:= main.cpp		\
			   genyd.cpp		\
			   socket.cpp

LOCAL_MODULE		:= genyd

LOCAL_C_INCLUDES	:= $(KERNEL_HEADERS) bionic external/stlport/stlport

LOCAL_MODULE_TAGS	:= optional

LOCAL_CFLAGS		:= -Werror=format

LOCAL_SHARED_LIBRARIES	:= libcutils libnetutils liblog libstlport

include $(BUILD_EXECUTABLE)

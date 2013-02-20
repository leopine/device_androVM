LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= bd-vbox-prop.cpp

LOCAL_CFLAGS:=-O2 -g 
LOCAL_CFLAGS+=-DLINUX -DIN_RING3 -DGC_ARCH_BITS=32 -DIN_GUEST -DIN_GUEST_R3 -DIN_RT_R3 -DVBOX_WITH_HGCM -DRT_OS_LINUX 

LOCAL_C_INCLUDES += device/androVM/vbox86/virtualbox/include

LOCAL_MODULE:=bd-vbox-prop
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= androVM-prop.cpp

LOCAL_CFLAGS:=-O2 -g 
LOCAL_CFLAGS+=-DLINUX -DIN_RING3 -DVBOX_WITH_HGCM -DRT_OS_LINUX 

LOCAL_C_INCLUDES += device/androVM/vbox86/virtualbox/include

LOCAL_MODULE:=androVM-prop
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= bd-vbox-ctrl.cpp

LOCAL_CFLAGS:=-O2 -g 
LOCAL_CFLAGS+=-DLINUX -DIN_RING3 -DVBOX_WITH_HGCM -DRT_OS_LINUX 

LOCAL_C_INCLUDES += device/androVM/vbox86/virtualbox/include

LOCAL_MODULE:=bd-vbox-ctrl
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= androVM-vbox-sf.cpp

LOCAL_CFLAGS:=-O2 -g 
LOCAL_CFLAGS+=-DLINUX -DIN_RING3 -DIN_SUP_R3 -DVBOX_WITH_HGCM -DRT_OS_LINUX -DVBOX_WITH_SHARED_FOLDERS

LOCAL_C_INCLUDES += device/androVM/vbox86/virtualbox/include

LOCAL_MODULE:=androVM-vbox-sf
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

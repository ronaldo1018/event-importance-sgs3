LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/frameworks/native/include \
	$(LOCAL_PATH)/include/system/core/include

LOCAL_LDLIBS += -L$(LOCAL_PATH)/lib -llog -lutils -lbinder
LOCAL_CFLAGS += -Wall -Wextra -Wconversion -Werror -Wno-error=unused-parameter -Wno-error=conversion

LOCAL_SRC_FILES := \
	importance.c \
	netlink.c \
	thread.c \
	core.c \
	activity.c \
	touch.c \
	timer.c \
	vector.c \
	parse.c \
	debug.c \
	mytimerfd.c \
	my_sysinfo.c \
	device_events.c \
	IImportance.cpp \
	ImportanceClient.cpp

LOCAL_MODULE := importance
include $(BUILD_EXECUTABLE)

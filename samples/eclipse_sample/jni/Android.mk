LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include C:/soft/OpenCV-2.4.8-android-sdk/sdk/native/jni/OpenCV.mk

LOCAL_MODULE    := recog_sample
LOCAL_SRC_FILES := jni_part.cpp ../../../plate_recog_lib/engine.cpp ../../../plate_recog_lib/sym_recog.cpp ../../../plate_recog_lib/syms.cpp
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)

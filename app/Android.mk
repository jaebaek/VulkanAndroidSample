# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

APP_GLUE_DIR = $(ANDROID_NDK)/sources/android/native_app_glue

include $(CLEAR_VARS)

LOCAL_MODULE    := app-glue
LOCAL_SRC_FILES := $(APP_GLUE_DIR)/android_native_app_glue.c

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE    := vktuts
LOCAL_SRC_FILES := $(LOCAL_PATH)/src/main/jni/main.cpp \
	$(LOCAL_PATH)/src/main/jni/vkcompute.cpp \
	$(LOCAL_PATH)/src/main/jni/vulkan_wrapper.cpp

LOCAL_C_INCLUDES := $(APP_GLUE_DIR)
LOCAL_STATIC_LIBRARIES := app-glue
LOCAL_LDLIBS := log android
LOCAL_CPPFLAGS	:= -Werror -std=c++11 -DVK_USE_PLATFORM_ANDROID_KHR

include $(BUILD_SHARED_LIBRARY)

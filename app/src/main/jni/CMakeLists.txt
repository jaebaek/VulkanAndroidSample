cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${APP_GLUE_DIR}
    ${ANDROID_NDK}/sources/third_party/vulkan/src/include)
add_library( app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)

add_library(vktuts SHARED
            main.cpp
            compute.cpp
            vkcompute.cpp
            vulkan_wrapper.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -std=c++11 \
                     -DVK_USE_PLATFORM_ANDROID_KHR \
                     -DUSE_DEBUG_EXTENTIONS")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(vktuts app-glue log android)

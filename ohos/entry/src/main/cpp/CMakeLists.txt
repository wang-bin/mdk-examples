cmake_minimum_required(VERSION 3.4.1)
project(player_napi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MDK SDK location.
# Download the OHOS MDK SDK from https://sourceforge.net/projects/mdk-sdk/files/nightly/
# and place it at entry/src/main/cpp/mdk-sdk (or set MDK_SDK_DIR).
if(NOT MDK_SDK_DIR)
    set(MDK_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mdk-sdk)
endif()

add_library(player_napi SHARED
    player_napi.cpp
)

target_include_directories(player_napi PRIVATE
    ${MDK_SDK_DIR}/include
)

# OHOS_ARCH is set by the build system (e.g. arm64-v8a, x86_64)
if(NOT DEFINED OHOS_ARCH)
    set(OHOS_ARCH "arm64-v8a")
endif()

target_link_libraries(player_napi
    libace_napi.z.so
    libhilog_ndk.z.so
    ${MDK_SDK_DIR}/lib/ohos/${OHOS_ARCH}/libmdk.so
)

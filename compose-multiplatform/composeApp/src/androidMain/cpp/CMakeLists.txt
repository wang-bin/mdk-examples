cmake_minimum_required(VERSION 3.22.1)
project(mdk_player_jni CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── MDK SDK location ────────────────────────────────────────────────────────
# Download the Android MDK SDK from:
#   https://sourceforge.net/projects/mdk-sdk/files/nightly/
# and extract it to src/androidMain/cpp/mdk-sdk/
# (or pass -DMDK_SDK_DIR=/absolute/path to cmake)
if(NOT MDK_SDK_DIR)
    set(MDK_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mdk-sdk)
endif()

# FindMDK.cmake sets up the `mdk` imported target (adds include dirs, links
# the prebuilt libmdk.so and arranges for it to be packaged with the APK).
include(${MDK_SDK_DIR}/lib/cmake/FindMDK.cmake)

add_library(mdk_player_jni SHARED MdkPlayerJNI.cpp)

target_link_libraries(mdk_player_jni
    mdk           # MDK SDK imported target from FindMDK.cmake
    android       # ANativeWindow_fromSurface / ANativeWindow_release
    log
)

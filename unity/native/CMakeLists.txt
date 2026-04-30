# MDK Unity Plugin — Native Plugin CMake Build
#
# Builds a shared library (DLL / dylib / .so / .a) wrapping the MDK SDK
# for use as a Unity native plugin on all supported desktop and mobile platforms.
#
# Usage:
#   cmake -B build -DMDK_SDK_DIR=/path/to/mdk-sdk
#   cmake --build build
#
# Or if building inside the mdk-examples tree where mdk-sdk/ is a sibling:
#   cmake -B build
#   cmake --build build
#
cmake_minimum_required(VERSION 3.16)
project(mdk-unity-plugin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# -------------------------------------------------------------------------
# Locate MDK SDK
# -------------------------------------------------------------------------
if(NOT MDK_SDK_DIR)
    # default: sibling mdk-sdk/ directory (when inside the mdk-examples repo)
    set(MDK_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../mdk-sdk")
endif()

set(MDK_CMAKE "${MDK_SDK_DIR}/lib/cmake/FindMDK.cmake")
if(NOT EXISTS "${MDK_CMAKE}")
    message(FATAL_ERROR
        "MDK SDK not found at '${MDK_SDK_DIR}'.\n"
        "Download it from https://sourceforge.net/projects/mdk-sdk/files/nightly/ "
        "and pass -DMDK_SDK_DIR=/path/to/mdk-sdk to cmake.")
endif()
include("${MDK_CMAKE}")

# -------------------------------------------------------------------------
# Platform-specific library type
#   iOS / tvOS  → static framework (Unity requires static libs on iOS)
#   All others  → shared library
# -------------------------------------------------------------------------
if(IOS OR TVOS)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(mdk-unity ${LIB_TYPE}
    MDKPlayer.cpp
)

target_include_directories(mdk-unity PRIVATE
    "${MDK_SDK_DIR}/include"
)

target_compile_definitions(mdk-unity PRIVATE MDK_UNITY_BUILD)

target_link_libraries(mdk-unity PRIVATE ${MDK_LIBRARIES})

# -------------------------------------------------------------------------
# Platform output names / install paths
# -------------------------------------------------------------------------
set_target_properties(mdk-unity PROPERTIES
    OUTPUT_NAME "mdk-unity"
    PREFIX ""
)

if(WIN32)
    # Unity expects the DLL in Assets/Plugins/Windows/x86_64/
    set_target_properties(mdk-unity PROPERTIES SUFFIX ".dll")
elseif(APPLE)
    if(IOS)
        # Static library for iOS — no suffix needed by Unity
        set_target_properties(mdk-unity PROPERTIES
            SUFFIX ".a"
        )
    else()
        # macOS bundle or dylib; Unity loads either
        set_target_properties(mdk-unity PROPERTIES SUFFIX ".bundle")
        set_target_properties(mdk-unity PROPERTIES MACOSX_BUNDLE TRUE)
    endif()
elseif(ANDROID)
    set_target_properties(mdk-unity PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
else()
    # Linux
    set_target_properties(mdk-unity PROPERTIES
        PREFIX ""
        SUFFIX ".so"
    )
endif()

# -------------------------------------------------------------------------
# Install target — mirrors Unity Plugins folder layout
# -------------------------------------------------------------------------
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(UNITY_PLUGIN_DEST "Plugins/Windows/x86_64")
elseif(WIN32)
    set(UNITY_PLUGIN_DEST "Plugins/Windows/x86")
elseif(APPLE AND IOS)
    set(UNITY_PLUGIN_DEST "Plugins/iOS")
elseif(APPLE)
    set(UNITY_PLUGIN_DEST "Plugins/macOS")
elseif(ANDROID)
    if(CMAKE_ANDROID_ARCH_ABI)
        set(UNITY_PLUGIN_DEST "Plugins/Android/libs/${CMAKE_ANDROID_ARCH_ABI}")
    else()
        set(UNITY_PLUGIN_DEST "Plugins/Android/libs/arm64-v8a")
    endif()
else()
    set(UNITY_PLUGIN_DEST "Plugins/Linux/x86_64")
endif()

install(TARGETS mdk-unity
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/${UNITY_PLUGIN_DEST}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/${UNITY_PLUGIN_DEST}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/${UNITY_PLUGIN_DEST}"
)

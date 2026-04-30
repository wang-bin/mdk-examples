cmake_minimum_required(VERSION 3.16)
project(mdk_shim C)

# ─── MDK SDK location ────────────────────────────────────────────────────────
# Download the desktop MDK SDK from:
#   https://sourceforge.net/projects/mdk-sdk/files/nightly/
# and extract it to a directory, then pass -DMDK_SDK_DIR=/path/to/mdk-sdk
# (or set the MDK_SDK_DIR cmake cache variable in your IDE).
if(NOT MDK_SDK_DIR)
    message(FATAL_ERROR
        "MDK_SDK_DIR is not set.\n"
        "Download the MDK SDK from https://sourceforge.net/projects/mdk-sdk/files/nightly/\n"
        "then re-run cmake with -DMDK_SDK_DIR=/path/to/mdk-sdk")
endif()

include(${MDK_SDK_DIR}/lib/cmake/FindMDK.cmake)

add_library(mdk_shim SHARED mdk_shim.c)

target_include_directories(mdk_shim PRIVATE ${MDK_SDK_DIR}/include)

target_link_libraries(mdk_shim PRIVATE mdk)

# On macOS, set the install name so the library can be found at runtime
if(APPLE)
    set_target_properties(mdk_shim PROPERTIES
        INSTALL_NAME_DIR "@rpath"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# Install to a convenient location for the Gradle application task
install(TARGETS mdk_shim
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION lib
)

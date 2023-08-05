
cmake_minimum_required(VERSION 3.16)
project(mdk-examples  VERSION 0.0.0)

if(POLICY CMP0063) # visibility. since 3.3
  cmake_policy(SET CMP0063 NEW)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXTRA_LIB_DIRS_FLAG)
  link_libraries(${EXTRA_LIB_DIRS_FLAG})
  list(APPEND CMAKE_REQUIRED_LIBRARIES ${EXTRA_LIB_DIRS_FLAG})
endif()
include_directories(${EXTRA_INCLUDE})
#link_directories(${EXTRA_LIB_DIRS_FLAG}) # why will append -L.../examples and become invalid -L..../examples-L....external...?
foreach(LFLAG ${EXTRA_LIB_DIRS_FLAG})
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
endforeach()
#list(APPEND CMAKE_REQUIRED_INCLUDES ${EXTRA_INCLUDE})
#list(APPEND CMAKE_REQUIRED_LIBRARIES ${EXTRA_LIB_DIRS_FLAG})
if(WIN32)
  set(APPTYPE WIN32) # /subsystem:windows
endif()
if(MSVC)# AND NOT CMAKE_CXX_SIMULATE_ID MATCHES MSVC) # link.exe, lld>=8.0
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -entry:mainCRTStartup")
endif()

include(CheckLibraryExists)

if(NOT BUILD_MDK)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/mdk-sdk/lib/cmake)
  #find_package(mdk REQUIRED)
  include(${CMAKE_SOURCE_DIR}/mdk-sdk/lib/cmake/FindMDK.cmake)
endif()
message("MDK_LIBRARIES:${MDK_LIBRARIES}")
add_subdirectory(GLFW)
add_subdirectory(Native)
add_subdirectory(SDL)
if(APPLE AND NOT IOS)
    add_subdirectory(macOS)
endif()
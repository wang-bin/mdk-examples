
cmake_minimum_required(VERSION 3.0)
project(mdk-examples  VERSION 0.0.0)

if(POLICY CMP0063) # visibility. since 3.3
  cmake_policy(SET CMP0063 NEW)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(NOT WIN32 AND NOT CMAKE_CROSSCOMPILING AND EXISTS /usr/local/include)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  list(APPEND CMAKE_REQUIRED_INCLUDES /usr/local/include)
  list(APPEND CMAKE_REQUIRED_LIBRARIES -L/usr/local/lib)
endif()

if(WIN32)
  set(APPTYPE WIN32) # /subsystem:windows
endif()

include(CheckLibraryExists)

if(NOT BUILD_MDK)
#list(APPEND CMAKE_PREFIX_PATH sdk/lib/cmake/mdk)
  find_package(mdk REQUIRED)
endif()

add_subdirectory(GLFW)
add_subdirectory(Native)
add_subdirectory(SDL)
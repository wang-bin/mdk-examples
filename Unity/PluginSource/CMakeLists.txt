cmake_minimum_required(VERSION 3.25)
project(MDKUnityPlugin VERSION 0.1.0 LANGUAGES CXX)
if(APPLE)
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(MDK_UNITY_RUNTIME_DIR "" CACHE PATH "Directory containing MDK and FFmpeg runtime libraries")
set(MDK_UNITY_INSTALL_SUBDIR "Assets/Plugins" CACHE STRING "Install destination relative to CMAKE_INSTALL_PREFIX")

if(NOT TARGET mdk)
  if(MDK_SDK_ROOT)
    file(TO_CMAKE_PATH "${MDK_SDK_ROOT}" MDK_SDK_ROOT_CMAKE)
    list(PREPEND CMAKE_MODULE_PATH "${MDK_SDK_ROOT_CMAKE}/lib/cmake")
    list(APPEND CMAKE_PREFIX_PATH "${MDK_SDK_ROOT_CMAKE}")
  endif()
  list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_LIST_DIR}/../../lib/cmake"
    "${CMAKE_CURRENT_LIST_DIR}/../../../cmake")
  find_package(MDK REQUIRED)
endif()
if(NOT MDK_UNITY_RUNTIME_DIR AND MDK_RUNTIME_DIR)
  set(MDK_UNITY_RUNTIME_DIR "${MDK_RUNTIME_DIR}")
endif()

set(MDK_UNITY_SOURCES
  MDKUnityPlugin.h
  RenderingPlugin.cpp
  BackendDispatch.cpp
  RenderAPI_OpenGL.cpp
)

if(WIN32)
  list(APPEND MDK_UNITY_SOURCES RenderAPI_D3D.cpp RenderAPI_Vulkan.cpp)
elseif(APPLE)
  list(APPEND MDK_UNITY_SOURCES RenderAPI_Metal.mm)
elseif(ANDROID OR CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND MDK_UNITY_SOURCES RenderAPI_Vulkan.cpp)
endif()

if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
  add_library(MDKUnityPlugin STATIC ${MDK_UNITY_SOURCES})
elseif(APPLE)
  add_library(MDKUnityPlugin MODULE ${MDK_UNITY_SOURCES})
else()
  add_library(MDKUnityPlugin SHARED ${MDK_UNITY_SOURCES})
endif()

target_include_directories(MDKUnityPlugin PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}/Unity"
  "${CMAKE_CURRENT_LIST_DIR}/../../.."
)
find_package(Threads REQUIRED)
target_link_libraries(MDKUnityPlugin PRIVATE mdk)
target_link_libraries(MDKUnityPlugin PRIVATE Threads::Threads)
target_compile_definitions(MDKUnityPlugin PRIVATE MDK_UNITY_PLUGIN=1)
if(WIN32)
  target_compile_definitions(MDKUnityPlugin PRIVATE NOMINMAX)
endif()

if(WIN32)
  target_link_libraries(MDKUnityPlugin PRIVATE d3d11 d3d12 dxgi opengl32)
endif()
if(MSVC)
  target_compile_options(MDKUnityPlugin PRIVATE /EHs-c- /GR-)
else()
  target_compile_options(MDKUnityPlugin PRIVATE -fno-exceptions -fno-rtti)
endif()

if(ANDROID)
  target_compile_definitions(MDKUnityPlugin PRIVATE UNITY_ANDROID=1)
  target_link_libraries(MDKUnityPlugin PRIVATE GLESv3 log android)
elseif(APPLE)
  find_library(METAL_FRAMEWORK Metal REQUIRED)
  find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
  target_link_libraries(MDKUnityPlugin PRIVATE ${METAL_FRAMEWORK} ${QUARTZCORE_FRAMEWORK})
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    find_library(OPENGLES_FRAMEWORK OpenGLES REQUIRED)
    target_link_libraries(MDKUnityPlugin PRIVATE ${OPENGLES_FRAMEWORK})
  else()
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(MDKUnityPlugin PRIVATE ${OPENGL_FRAMEWORK})
  endif()
elseif(UNIX)
  find_package(OpenGL REQUIRED)
  target_link_libraries(MDKUnityPlugin PRIVATE OpenGL::GL)
endif()

if(WIN32 OR CMAKE_SYSTEM_NAME MATCHES "Linux")
  find_package(Vulkan REQUIRED)
  target_include_directories(MDKUnityPlugin PRIVATE ${Vulkan_INCLUDE_DIRS})
elseif(ANDROID)
  find_path(ANDROID_VULKAN_INCLUDE_DIR vulkan/vulkan.h)
  if(ANDROID_VULKAN_INCLUDE_DIR)
    target_include_directories(MDKUnityPlugin PRIVATE ${ANDROID_VULKAN_INCLUDE_DIR})
  endif()
endif()

set_target_properties(MDKUnityPlugin PROPERTIES
  OUTPUT_NAME "MDKUnityPlugin"
  CXX_VISIBILITY_PRESET default
  VISIBILITY_INLINES_HIDDEN OFF
)
if(WIN32)
  set_target_properties(MDKUnityPlugin PROPERTIES PREFIX "")
elseif(APPLE AND NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set_target_properties(MDKUnityPlugin PROPERTIES PREFIX "" SUFFIX ".bundle")
  set_target_properties(MDKUnityPlugin PROPERTIES
    BUILD_RPATH "@loader_path"
    INSTALL_RPATH "@loader_path")
elseif(UNIX AND NOT ANDROID)
  set_target_properties(MDKUnityPlugin PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN")
endif()

install(TARGETS MDKUnityPlugin
  RUNTIME DESTINATION "${MDK_UNITY_INSTALL_SUBDIR}"
  LIBRARY DESTINATION "${MDK_UNITY_INSTALL_SUBDIR}"
  ARCHIVE DESTINATION "${MDK_UNITY_INSTALL_SUBDIR}")

if(MDK_UNITY_RUNTIME_DIR)
  file(GLOB MDK_UNITY_RUNTIME_FILES CONFIGURE_DEPENDS
    "${MDK_UNITY_RUNTIME_DIR}/*.dll"
    "${MDK_UNITY_RUNTIME_DIR}/*.so"
    "${MDK_UNITY_RUNTIME_DIR}/*.so.*"
    "${MDK_UNITY_RUNTIME_DIR}/*.dylib")
  if(MDK_UNITY_RUNTIME_FILES)
    install(FILES ${MDK_UNITY_RUNTIME_FILES} DESTINATION "${MDK_UNITY_INSTALL_SUBDIR}")
  endif()
  file(GLOB MDK_UNITY_RUNTIME_BUNDLES CONFIGURE_DEPENDS
    "${MDK_UNITY_RUNTIME_DIR}/*.bundle"
    "${MDK_UNITY_RUNTIME_DIR}/*.framework")
  if(MDK_UNITY_RUNTIME_BUNDLES)
    install(DIRECTORY ${MDK_UNITY_RUNTIME_BUNDLES} DESTINATION "${MDK_UNITY_INSTALL_SUBDIR}")
  endif()
endif()

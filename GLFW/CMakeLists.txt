
if(CMAKE_CROSSCOMPILING)
  find_package(Vulkan NO_CMAKE_SYSTEM_PATH) #cmake 3.7+. cross build win on linux may include /usr/include, which is not desired
else()
  find_package(Vulkan)
endif()

#find_package(glfw)
set(CMAKE_REQUIRED_LIBRARIES_OLD ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES_OLD ${CMAKE_REQUIRED_INCLUDES})
set(GLFW_NAME glfw)
if(WIN32)
  set(GLFW_NAME glfw3)
endif()
if(WINDOWS_DESKTOP)# AND MSVC)
  set(GLFW_DEP gdi32 shell32 user32) # Gdi32.Lib, shell32.lib, User32.Lib. but cmake treates .Lib as .Lib.lib
elseif(APPLE)
  set(GLFW_DEP "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation" "-framework CoreVideo")
endif()
list(PREPEND CMAKE_REQUIRED_LIBRARIES ${GLFW_DEP}) # find libglfw.tbd before libglfw.dylib(single arch) in /usr/local/lib
check_library_exists(${GLFW_NAME} glfwInit "" HAVE_GLFW3)
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_OLD})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_OLD})
if(HAVE_GLFW3)
  add_executable(glfwplay ${APPTYPE} glfwplay.cpp)# ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
  if (EXISTS ${Vulkan_INCLUDE_DIR})
    target_include_directories(glfwplay PRIVATE ${Vulkan_INCLUDE_DIR})
  endif()
  target_link_libraries(glfwplay PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  add_executable(PlaylistAsOne ${APPTYPE} PlayListAsOne.cpp)# ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
  if (EXISTS ${Vulkan_INCLUDE_DIR})
    target_include_directories(PlaylistAsOne PRIVATE ${Vulkan_INCLUDE_DIR})
  endif()
  target_link_libraries(PlaylistAsOne PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  add_executable(multiplayers ${APPTYPE} multiplayers.cpp)
  target_link_libraries(multiplayers PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  add_executable(multiwindows ${APPTYPE} multiwindows.cpp)
  target_link_libraries(multiwindows PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  install(TARGETS PlaylistAsOne glfwplay multiplayers multiwindows RUNTIME DESTINATION bin)
endif()

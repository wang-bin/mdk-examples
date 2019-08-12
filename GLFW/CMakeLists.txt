
#find_package(glfw)
set(CMAKE_REQUIRED_LIBRARIES_OLD ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES_OLD ${CMAKE_REQUIRED_INCLUDES})
set(GLFW_NAME glfw)
if(WIN32)
  set(GLFW_NAME glfw3)
endif()
if(WINDOWS_DESKTOP AND MSVC)
  set(GLFW_DEP gdi32 shell32 user32) # Gdi32.Lib, shell32.lib, User32.Lib. but cmake treates .Lib as .Lib.lib
elseif(APPLE)
  set(GLFW_DEP "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation" "-framework CoreVideo")
endif()
list(APPEND CMAKE_REQUIRED_LIBRARIES ${GLFW_DEP})
check_library_exists(${GLFW_NAME} glfwInit "" HAVE_GLFW3)
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_OLD})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_OLD})
if(HAVE_GLFW3)
  add_executable(glfwplay ${APPTYPE} glfwplay.cpp)# ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
  target_link_libraries(glfwplay PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  add_executable(multiplayers ${APPTYPE} multiplayers.cpp)
  target_link_libraries(multiplayers PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  add_executable(multiwindows ${APPTYPE} multiwindows.cpp)
  target_link_libraries(multiwindows PRIVATE mdk ${GLFW_NAME} ${GLFW_DEP})

  if(MSVC)# AND NOT CMAKE_CXX_SIMULATE_ID MATCHES MSVC) # link.exe, lld>=8.0
    target_link_libraries(glfwplay PRIVATE -entry:mainCRTStartup)
    target_link_libraries(multiplayers PRIVATE -entry:mainCRTStartup)
    target_link_libraries(multiwindows PRIVATE -entry:mainCRTStartup)
  endif()
  install(TARGETS glfwplay multiplayers multiwindows RUNTIME DESTINATION bin)
endif()

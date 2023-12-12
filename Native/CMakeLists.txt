if(NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "tvOS")
  foreach(tgt mdkplay offscreen DecodeFps
    MediaInfoReader
    Thumbnail
  )
    add_executable(${tgt} ${APPTYPE} ${tgt}.cpp)
    target_link_libraries(${tgt} PRIVATE mdk)
  endforeach()
  install(TARGETS
    DecodeFps
    Thumbnail
#    mdkplay
    RUNTIME DESTINATION bin)
endif()

#set(OpenGL_GL_PREFERENCE GLVND)
include(FindOpenGL)
if(OpenGL_GLX_FOUND)
  add_executable(x11play x11play.cpp)
  target_include_directories(x11play PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${X11_X11_INCLUDE_PATH})
  target_link_libraries(x11play mdk ${X11_X11_LIB} ${CMAKE_LIBRARY_PATH_FLAG}${X11_X11_LIB}/.. GL)
  install(TARGETS x11play RUNTIME DESTINATION bin)
endif()

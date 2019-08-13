
add_executable(offscreen ${APPTYPE} offscreen.cpp)
target_link_libraries(offscreen PRIVATE mdk)

if(NOT IOS)
  add_executable(mdkplay ${APPTYPE} mdkplay.cpp)
  target_link_libraries(mdkplay PRIVATE mdk)
endif()

find_package(X11)
if(X11_FOUND)
  add_executable(x11play x11play.cpp)
  target_include_directories(x11play PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${X11_X11_INCLUDE_PATH})
  target_link_libraries(x11play mdk ${X11_X11_LIB} ${CMAKE_LIBRARY_PATH_FLAG}${X11_X11_LIB}/.. GL)
  install(TARGETS x11play RUNTIME DESTINATION bin)
endif()

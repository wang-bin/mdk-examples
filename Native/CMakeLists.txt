
if(NOT IOS)
  add_executable(mdkplay ${APPTYPE} mdkplay.cpp)
  target_link_libraries(mdkplay PRIVATE mdk)
  if(MSVC)# AND NOT CMAKE_CXX_SIMULATE_ID MATCHES MSVC) # link.exe, lld>=8.0
    target_link_libraries(mdkplay PRIVATE -entry:mainCRTStartup)
  endif()
endif()

find_package(X11)
if(X11_FOUND)
  add_executable(x11play x11play.cpp)
  target_include_directories(x11play PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${X11_X11_INCLUDE_PATH})
  target_link_libraries(x11play mdk ${X11_X11_LIB} ${CMAKE_LIBRARY_PATH_FLAG}${X11_X11_LIB}/.. GL)
  install(TARGETS x11play RUNTIME DESTINATION bin)
endif()

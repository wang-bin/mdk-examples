
cmake_minimum_required(VERSION 3.0)
# see cmake/Tests/iOSNavApp
if(NOT APPLE OR IOS)
  return()
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9) # libc++

option(APP_BUNDLE "Build as bundle" ON)

if(APP_BUNDLE)
  set(APP_TYPE MACOSX_BUNDLE)
endif()
set(APP_NAME player)
set(RESOURCES_IN MainMenu.xib.in)
foreach(xib_in ${RESOURCES_IN})
  get_filename_component(xib_in_name ${xib_in} NAME_WE)
  configure_file(${xib_in} ${CMAKE_CURRENT_BINARY_DIR}/${xib_in_name}.xib)
  configure_file(${xib_in} ${CMAKE_CURRENT_SOURCE_DIR}/Player/Base.lproj/${xib_in_name}.xib) # used by xcode project
  list(APPEND RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/${xib_in_name}.xib)
endforeach()

add_executable(${APP_NAME} ${APP_TYPE}
  Player/main.mm
  Player/AppDelegate.mm
  Player/AppDelegate.h
  ${RESOURCES}
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${RPATH_FLAGS} -Wl,-rpath,@executable_path/../Frameworks -Wl,-rpath,/usr/local/lib")
# assume mdk.framework is in current dir
target_compile_options(${APP_NAME} PRIVATE -F${CMAKE_CURRENT_SOURCE_DIR}) # -fobjc-arc) # arc: _objc_retainAutoreleasedReturnValue is not defined in osx10.6 objc runtime. Using new libobjc on osx10.6 will result in CoreFoundation error
# lflag -fobjc-arc/-fobjc-link-runtime: opensource clang missing libarclite_macosx.a, need link to system libarclite_macosx.a
target_link_libraries(${APP_NAME} "-framework AppKit" "-framework QuartzCore" "-framework OpenGL" -F${CMAKE_CURRENT_SOURCE_DIR} mdk) # AppKit or Cocoa. "-framework QuartzCore" for CAOpenGLLayer
if(APP_BUNDLE)
  target_compile_definitions(${APP_NAME} PRIVATE APP_BUNDLE)
  set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mdk.macos.${APP_NAME}"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    RESOURCE "${RESOURCES}" # doesn't work, just copy
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTIFY "Mac Developer"
    XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET}
  )
  foreach(xib ${RESOURCES})
  get_filename_component(xib_name ${xib} NAME_WE)
  add_custom_command(TARGET ${APP_NAME} POST_BUILD
  # output-format: human-readable-text, xml1
    COMMAND ibtool --errors --warnings --module ${APP_NAME} --target-device mac --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET} --output-format human-readable-text
    --compile ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources/${xib_name}.nib ${xib}
  )
  endforeach()
endif()

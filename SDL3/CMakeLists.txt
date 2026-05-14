find_package(SDL3 QUIET CONFIG)

if(TARGET SDL3::SDL3)
  set(SDL3_TARGET SDL3::SDL3)
elseif(TARGET SDL3::SDL3-shared)
  set(SDL3_TARGET SDL3::SDL3-shared)
elseif(TARGET SDL3::SDL3-static)
  set(SDL3_TARGET SDL3::SDL3-static)
endif()

if(SDL3_TARGET)
  foreach(tgt sdl3play)
    add_executable(${tgt} ${APPTYPE} ${tgt}.cpp)
    target_link_libraries(${tgt} PRIVATE mdk ${SDL3_TARGET})
    if(NOT WIN32)
      target_link_libraries(${tgt} PRIVATE dl)
    endif()
    if(WIN32)
      target_link_libraries(${tgt} PRIVATE winmm imm32 ole32 version) # for static sdl
    elseif(APPLE)
      target_link_libraries(${tgt} PRIVATE iconv "-framework CoreFoundation" "-framework CoreGraphics" "-framework Foundation" "-framework AppKit" "-framework IOKit" "-framework AudioToolbox" "-framework CoreAudio" "-framework ForceFeedback" "-framework Metal" "-framework CoreVideo" "-framework CoreServices" "-framework Carbon"
      "-weak_framework CoreHaptics" "-weak_framework GameController"
      ) # for static sdl
    endif()
    install(TARGETS ${tgt} RUNTIME DESTINATION bin)
  endforeach()
endif()

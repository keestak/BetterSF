cmake_minimum_required(VERSION 3.25)
project(BetterSF VERSION 1.0.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FluidSynth.cmake)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    BetterSF.cpp
    BetterSF.h
    CustomControls.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  FORMATS
    MINIMAL_PLUGINS APP VST3 CLAP
  LINK
    libfluidsynth
)

if(WIN32)
  set(_rc_file "${CMAKE_CURRENT_SOURCE_DIR}/resources/main.rc")

  target_sources(BetterSF-vst3 PRIVATE "${_rc_file}")
  target_sources(BetterSF-clap PRIVATE "${_rc_file}")

  set_source_files_properties("${_rc_file}" PROPERTIES
    COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/img\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
  )
endif()
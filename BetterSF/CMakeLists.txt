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
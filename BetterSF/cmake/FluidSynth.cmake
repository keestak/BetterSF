include(FetchContent)

FetchContent_Declare(
    fluidsynth
    GIT_REPOSITORY https://github.com/FluidSynth/fluidsynth.git
    GIT_TAG v2.6.0
    GIT_SHALLOW TRUE
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(enable-alsa OFF CACHE BOOL "" FORCE)
set(enable-aufile OFF CACHE BOOL "" FORCE)
set(enable-dsound OFF CACHE BOOL "" FORCE)
set(enable-jack OFF CACHE BOOL "" FORCE)
set(enable-ladspa OFF CACHE BOOL "" FORCE)
set(enable-network OFF CACHE BOOL "" FORCE)
set(enable-pulseaudio OFF CACHE BOOL "" FORCE)
set(enable-waveout OFF CACHE BOOL "" FORCE)
set(enable-wasapi OFF CACHE BOOL "" FORCE)
set(enable-winmidi OFF CACHE BOOL "" FORCE)

set(enable-readline OFF CACHE BOOL "" FORCE)
set(enable-openmp OFF CACHE BOOL "" FORCE)
set(enable-native-dls OFF CACHE BOOL "" FORCE)
set(enable-signalsmith OFF CACHE BOOL "" FORCE)

set(enable-libinstpatch OFF CACHE BOOL "" FORCE)
set(enable-ipv6 OFF CACHE BOOL "" FORCE)
set(enable-libsndfile OFF CACHE BOOL "" FORCE)

set(enable-floats ON CACHE BOOL "" FORCE)

set(osal cpp11 CACHE STRING "" FORCE)

FetchContent_MakeAvailable(fluidsynth)
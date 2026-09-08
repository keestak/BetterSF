# BetterSF
A soundfont (sf2) loader and player meant for use as a plugin instrument.
This software is built with [iPlug2](https://github.com/iplug2/iplug2) and [Fluidsynth](https://github.com/FluidSynth/fluidsynth).

***
# Building from source

Clone the repo and init submodules. Then under iPlug2 run download-iplug-sdks.h and download-prebuilt-libs.h to download iPlug2's dependencies.

Setup CMake build with:
```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
```
Build with CMake using:
```bash
cmake --build build --config Release
```

To build a specific target, use:
```bash
cmake --build ./build --config Release --target BetterSF-app
cmake --build ./build --config Release --target BetterSF-vst3
cmake --build ./build --config Release --target BetterSF-clap
```

This will automatically download and build fluidsynth which this project depends on, so it must be run at least once. Afterwards, you can open the BetterSF.sln file in visual studio to edit and build it.

***
# LICENSE
BetterSF uses the [MIT License](https://en.wikipedia.org/wiki/MIT_License) and is free, open source, non-commercial software.

BetterSF uses [iPlug2](https://github.com/iplug2/iplug2), which uses a [zlib-like License](https://github.com/iPlug2/iPlug2/blob/master/LICENSE.txt).

BetterSF uses [Fluidsynth](https://github.com/FluidSynth/fluidsynth)
, which uses the [LGPL-2.1 License](https://github.com/FluidSynth/fluidsynth/blob/master/LICENSE).

This software statically links to fluidsynth, which is fetched and built via CMake.

***

This software was developed without the use of AI tools and I would like to keep it that way.

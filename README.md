# Horizons-Fetch: Little program to fetch JPL Horizons celestial body data.

[Original document location](https://jausoft.com/cgit/horizons-fetch.git/about/).

## Git Repository
This project's canonical repositories is hosted on [Gothel Software](https://jausoft.com/cgit/horizons-fetch.git/).

## Goals
*Horizons-Fetch* is just a little program to fetch celestial body data from [JPL Horizons](https://ssd.jpl.nasa.gov/horizons/).

### JPL Horizons Links
- [Horizons Manual](https://ssd.jpl.nasa.gov/horizons/manual.html)
- [Horizons API](https://ssd-api.jpl.nasa.gov/doc/horizons.html)
- [Horizons API File / POST](https://ssd-api.jpl.nasa.gov/doc/horizons_file.html)
- [Orbit Viewer](https://cneos.jpl.nasa.gov/ov/index.html#)

## Related Work
[gfxbox2](https://jausoft.com/cgit/cs_class/gfxbox2.git/about/)'s 
[Solarsystem WebApp](https://jausoft.com/projects/gfxbox2/solarsystem.html) - ([Sonnensystem Simulation](https://jausoft.com/cgit/cs_class/gfxbox2.git/plain/doc/Sonnensystem.pdf)).

## Supported Platforms
- C++20 or better, see [jaulib C++ Minimum Requirements](https://jausoft.com/cgit/jaulib.git/about/README.md#cpp_min_req).
- [libcurl4](https://curl.se/libcurl/)

### Build Dependencies
- CMake >= 3.21 (2021-07-14)
- C++ compiler
  - gcc >= 11 (C++20), recommended >= 12.2.0
  - clang >= 13 (C++20), recommended >= 18.1.6
- Optional for `lint` validation
  - clang-tidy >= 18.1.6
- Optional for `eclipse` and `vscodium` integration
  - clangd >= 18.1.6
  - clang-tools >= 18.1.6
  - clang-format >= 18.1.6
- Optional
  - libunwind8 >= 1.2.1
  - libcurl4 >= 7.74 (tested, lower may work)
- [jaulib](https://jausoft.com/cgit/jaulib.git/about/) *submodule*

#### Install on Debian or Ubuntu

Installing build dependencies for Debian >= 12 and Ubuntu >= 22:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
apt install git
apt install build-essential g++ gcc libc-dev libpthread-stubs0-dev 
apt install clang-18 clang-tidy-18 clangd-18 clang-tools-18 clang-format-18
apt install libunwind8 libunwind-dev
apt install libcurl4 libcurl4-gnutls-dev
apt install cmake cmake-extras extra-cmake-modules pkg-config
apt install doxygen graphviz
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If using optional clang toolchain, 
perhaps change the clang version-suffix of above clang install line to the appropriate version.

After complete clang installation, you might want to setup the latest version as your default.
For Debian you can use this [clang alternatives setup script](https://jausoft.com/cgit/jaulib.git/tree/scripts/setup_clang_alternatives.sh).

### Build Procedure

#### Build preparations

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
git clone --recurse-submodules git://jausoft.com/srv/scm/horizons-fetch.git
cd horizons-fetch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

<a name="cmake_presets_optional"></a>

#### CMake Build via Presets
See [jaulib CMake build presets](https://jausoft.com/cgit/jaulib.git/about/README.md#cmake_presets_optional) ...

Kick-off the workflow by e.g. using preset `release-gcc` to configure, build, test, install and building documentation.
You may skip `install` and `doc` by dropping it from `--target`.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
cmake --preset release-gcc
cmake --build --preset release-gcc --parallel
cmake --build --preset release-gcc --target test install doc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may utilize `scripts/build-preset.sh` for an initial build, install and test workflow.


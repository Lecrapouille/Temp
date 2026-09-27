# Compilation and installation

Compages is a C++20 project developed primarily on Linux. Its current `GL45`
backend requires an OpenGL 4.5 core context. Apple only exposes OpenGL 4.1, so
macOS can compile Compages and run its CPU tests, but GPU tests are skipped and
the example gallery cannot run there. Windows is not currently maintained.

In the commands below, adapt `-j8` to the number of logical CPU cores on your
machine.

## System prerequisites

The compiler, Make, CMake, Git and `pkg-config` are required. GLFW is a system
dependency of the examples and GPU test harness; it is not a dependency of the
Compages library itself.

Ubuntu and Debian:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake git pkg-config libglfw3-dev
```

For the unit tests, also install `libgtest-dev libgmock-dev`. A headless Linux
machine needs `xvfb`, Mesa and `libgl1-mesa-dev` to run GPU tests and the gallery.

macOS with Homebrew:

```sh
brew install cmake glfw googletest pkg-config
```

GLEW and bzip2 are not dependencies of the current implementation.

## Build

The recursive clone retrieves the MyMakefile build machinery. The other
third-party sources are described by `external/manifest` and downloaded into
`external/` by the first Make target:

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make download-external-libs
make -j8 all
```

`compile-external-libs` builds ReactPhysics3D, which only the physics parked in
`attic/Physics/` needs; the library and the gallery do not link it. The main
build creates static and shared Compages libraries in `build/` and also builds
the gallery through the top-level `post-build` target. Run `make help` to list
the available targets and variables.

## Examples

The 43 examples in the current numbered grid are compiled into one gallery:

- `00_GettingStarted`: 16 examples, from `00a_Dummy` through `07_PointClouds`;
- `10_ScientificAndCompute`: 7 examples;
- `20_Performance`: 3 examples;
- `30_WorldAndAssets`: 13 examples;
- `50_Complete`: 4 examples, ending with the `53_DoomLike` game.

See [the examples guide](../examples/README.md) for the complete names. Build
the gallery explicitly, if the top-level build has not already done so:

```sh
make -j8 -C examples all
```

Run it from the repository root:

```sh
./build/Compages-examples                         # start at 00a_Dummy
./build/Compages-examples 04_DepthAndTransforms  # start at this example
./build/Compages-examples --check                 # all 43: smoke + leak test
./build/Compages-examples --check --shots /tmp/shots
```

Arrow keys switch examples, space hides the overlay and escape quits.
`--check` visits every example for a few frames and fails if one cannot run or
leaves a GPU resource behind. It requires OpenGL 4.5; on a headless Linux host,
run it under Xvfb and llvmpipe:

```sh
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 \
  MESA_GL_VERSION_OVERRIDE=4.5 MESA_GLSL_VERSION_OVERRIDE=450 \
  ./build/Compages-examples --check
```

## Installation layout

The defaults come from `.makefile/project/Makefile`, not from a distribution-
specific path:

- `PREFIX=/usr/local`;
- `INCLUDEDIR=$(PREFIX)/include`;
- `LIBDIR=$(PREFIX)/lib`;
- `PKGLIBDIR=$(LIBDIR)/pkgconfig`;
- `DATADIR=$(PREFIX)/share`.

Install with:

```sh
sudo make install
```

For version 0.10.0 and the default prefix, this installs:

- `libCompages` (static and shared) in `/usr/local/lib`;
- public headers from `include/`, plus the public EnTT and units headers, under
  `/usr/local/include/Compages/0.10.0/`;
- `Compages.pc` and the versioned `Compages-0.10.0.pc` in
  `/usr/local/lib/pkgconfig`;
- project documentation/data under `/usr/local/share/Compages/0.10.0/`.

The include root contains the `Compages/` directory, so consumers use
`#include <Compages/...>`. No header under `src/` is public or installed.

Override `PREFIX`, `INCLUDEDIR`, `LIBDIR`, `PKGLIBDIR`, `DATADIR` or `BINDIR`
on the Make command line when needed. `DESTDIR` stages files without changing
the paths recorded in the generated pkg-config file, for example:

```sh
make DESTDIR="$PWD/stage" PREFIX=/usr install
```

Do not assume `/usr/lib` or `/usr/include`: query the selected Make variables
or pkg-config. With the default installation:

```sh
pkg-config --modversion Compages
pkg-config --cflags --libs Compages
```

If `/usr/local/lib/pkgconfig` is not in the platform search path:

```sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
```

## Using the installed library

All public contracts live under `include/Compages/`. Prefer the umbrella header
or include only the required components:

```cpp
#include <Compages/Compages.hpp>
#include <Compages/GPU/GPU.hpp>
#include <Compages/World/World.hpp>
```

`gpu::init()` does not create a window or context. Pass the symbol resolver from
the toolkit which created the context, such as `glfwGetProcAddress`,
`SDL_GL_GetProcAddress` or the Qt equivalent.

Compile and link an installed consumer with the actual generated package name:

```sh
c++ -std=c++20 main.cpp -o app $(pkg-config --cflags --libs Compages)
```

For a fully static link, use `pkg-config --static`. At runtime, ensure the selected `LIBDIR` is visible
to the platform dynamic loader, or embed an rpath in the application.

## Unit tests

With GoogleTest and GoogleMock installed:

```sh
make -j8 -C tests all
./build/Compages-UnitTest
```

On headless Linux, use the same Xvfb/llvmpipe environment shown for the gallery.
Tests which need an unavailable OpenGL 4.5 context skip themselves; this is the
expected limitation on macOS. CPU tests still execute there.

For a local coverage report:

```sh
make -C tests coverage
```

## Third-party dependency model

- **EnTT 4.0.0** and **units 2.3.3** are header-only public dependencies.
  They are downloaded by `download-external-libs`; installation copies EnTT
  headers and `units.h` into the versioned Compages include root because public
  Compages headers include them.
- **ReactPhysics3D 0.10.2** is still downloaded and built by
  `compile-external-libs` for the physics parked in `attic/Physics/`. Nothing
  in the build links it.
- **cgltf**, **nlohmann/json** and **stb** are downloaded header-only
  implementation dependencies. They are used only while compiling Compages and
  are not installed as public headers.
- **glad 2.0.8** is vendored and compiled inside the `GL45` backend. Its headers
  remain private under `src/GPU/Backends/GL45/glad/`.
- **Dear ImGui** is downloaded source compiled only into the example gallery;
  it is not part of `libCompages` and is not installed.
- **GLFW** is supplied by the operating system and used only by the examples and
  GPU test harness. Applications may use another context/window toolkit.
- **Compages-data** is downloaded for example assets and data-dependent tests.
- **MyMakefile** is the Git submodule providing the build rules.

The former GLEW, SOIL, Bullet and bzip2 dependencies are not used.

Further reading: [tutorial](tutorials/Core.md),
[architecture](Architecture.md), [design](Design.md), and
[OpenGL traces](Traces.md).

# Compilation / Installation

OpenGLCppWrapper is primarily developed on Linux. macOS builds up to and
including OpenGL 4.1; the compute and DSA-based paths of the `gpu::` layer
require an environment that can supply OpenGL 4.5 (see [Design.md](Design.md)
on macOS). Windows is not currently maintained.

Note: In this doc the `-j8` with the `make` command is the number of your CPU
cores. Adapt to your case.

## Prerequisite

You need to install the following libs on your operating system: glfw glew.
- Ubuntu, Debian: `sudo apt-get update && apt-get install libglew-dev libglfw3-dev libbz2-dev`
- Mac OS X: `brew install glfw glew`

## Compilation of the API

To download the project, its external libraries and compile the API with its examples:

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/OpenGLCppWrapper.git --depth=1
cd OpenGLCppWrapper
make download-external-libs
make compile-external-libs
make -j8
```

The `--recurse-submodules` is important to get my Makefile routines for compiling the project.

If you are a developper `make download-external-libs` and `make compile-external-libs`
has to be called once or when you want to upgrade the libraries: they follow the
master branch and they remove the previously downloaded third-parts.

The `make download-external-libs` command plays the same role than a recursive git clone because
I hate git submodules: it always make things painful and `repo`
with its manifests is a bit overkill, so I prefer Makefile rules or script
shell and this gave me good result even with continuous integration.

After `make` a `build/` folder shall have been created containing the compiled
and runnable files. Two libraries (one static the second dynamic) shall also be
present. You can use them for your project. You can type `sudo make install` to
install them on your system.

If, after that, you want to modify code source, just do `make`. You can type
`make help` for displaying rules.

## Compilation of examples

The seventeen examples live in a single program that shares one window and one
overlay:

```sh
cd examples
make -j8
```

Run the gallery:
```sh
./build/OpenGLCppWrapper-examples                    # start at the first
./build/OpenGLCppWrapper-examples 05_IndexedCube     # start on that one
./build/OpenGLCppWrapper-examples --check            # smoke + leak test
./build/OpenGLCppWrapper-examples --shots /tmp/shots # one PNG per example
```

Arrows switch example, space hides the overlay, escape quits. The overlay's
"resources" counters should fall back to zero when an example is left; that is
the leak test the `--check` mode automates.

## Installation

Multiple versions of this library can coexist thanks to their versioning number.
After the compilation, just type:

```sh
cd OpenGLCppWrapper
sudo make install
```

This will install:
* in `/usr/lib`: the static and shared libraries libOpenGLCppWrapper.
* in `/usr/include/openglcppwrapper-<version>`: all headers files (hpp).
* in `/usr/lib/pkgconfig`: a pkg confile file for linking this API with your future projects.
* in `/usr/share/OpenGLCppWrapper/<version>/`: documentation, examples and other files.

If you do not like the default location, Pass to Makefile options `DESTDIR`,
`PREFIX` and `BINDIR` or directly edit the file `.makefile/Makefile.header`
(note: touching a makefile will force to recompile the whole project).

Check the presence of libraries in your system:
```sh
cd /usr/lib
ls -la libOpenGLCppWrapper*
```
Or better:
```
echo `pkg-config `openglcppwrapper --libs``
```

## Developpers

### How to use OpenGLCppWrapper in your project?

The public headers are the ones under `src/GPU/`, `src/Assets/`, `src/World/`,
`src/Scene/` and `src/Render/`. Include what you need directly, e.g.:

```cpp
#include <OpenGLCppWrapper/GPU/GPU.hpp>
#include <OpenGLCppWrapper/World/World.hpp>
#include <OpenGLCppWrapper/Scene/Scene.hpp>
#include <OpenGLCppWrapper/Render/Extractor.hpp>
#include <OpenGLCppWrapper/Render/Renderer.hpp>
```

`gpu::init` never opens a window. Pass it the driver-symbol loader that comes
from the toolkit that already opened the context (GLFW, SDL, Qt…):

```cpp
if (auto ready = gpu::init(&glfwGetProcAddress); !ready) {
    std::cerr << ready.error() << '\n';
    return EXIT_FAILURE;
}
```

Compile using pkg-config:
```sh
CFLAGS=`pkg-config openglcppwrapper --cflags`
LDFLAGS=`pkg-config openglcppwrapper --libs`
g++ -std=c++20 main.cpp -o prog $CFLAGS $LDFLAGS
```

**note:** You may need indicate where are shared libraries. For example on Mac OS X:

```sh
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:/your/path/to/your/lib/folder
```

Example with Makefile given in [here](https://github.com/Lecrapouille/LinkAgainstMyLibs/tree/master/OpenGL).

### What code to write in my project ?

* [Tutorial](tutorials/Core.md) — a short guided walk of both stacks.
* [Examples](../examples/README.md) — the numbered walkthrough, one idea per file.
* [Architecture](Architecture.md) — the map of the layers.
* [Design](Design.md) — the *why* of each choice.
* [Debug OpenGL](Traces.md)

### Doxygen

Documentation of the code source can be found [here](https://lecrapouille.github.io/OpenGLCppWrapper.github.io/).
It can be localy generated as `doc/html/index.html` by typing `make doc`.

### Unit tests

Tests depend on [googletest](https://github.com/google/googletest). Install
it once, then:

```sh
cd OpenGLCppWrapper/tests
make -j8
./build/OpenGLCppWrapper-UnitTest
```

The tests cover the `gpu::` layer against an offscreen context, the linear
algebra, `AABB`, `Frustum` and `Units` under `Math/`, and the full simulation
stack under `World/` (registry, spatial graph, transforms, components,
hierarchy) plus the extraction-to-draw pipeline in `WorldDrawTests.cpp`.

For a coverage report:

```sh
cd OpenGLCppWrapper/tests
make coverage
```

If all tests pass, a coverage report is written to `doc/coverage/` and the
`index.html` is opened automatically.

### Third-parts

This project depends on third-parts that are automatically downloaded with
`make download-external-libs`. They are compiled as static libraries with
`make compile-external-libs` but they are not installed on your operating
system. It's onnly when the `sudo make install` is called that their header
files are copied within the OpenGLCppWrapper header files.

Here the list of third-parties:
* [MyMakefile](https://github.com/Lecrapouille/MyMakefile) — the shared
  Makefile machinery, cloned as a git submodule (hence `--recurse-submodules`
  when you first clone).
* [OpenGLCppWrapper-data](https://github.com/Lecrapouille/OpenGLCppWrapper-data)
  — textures used by the examples, downloaded by the Makefile.
* [stb_image](https://github.com/nothings/stb) — image decoding, vendored under
  `src/GPU/Internal/`.
* [Dear ImGui](https://github.com/ocornut/imgui) — the overlay of the
  examples gallery, downloaded by `download-external-libs`.
* [units](https://github.com/nholthaus/units) — SI units for angles, distances
  and time. Downloaded by `download-external-libs`.
* [backward-cpp](https://github.com/bombela/backward-cpp) — stack traces in
  debug builds. Downloaded by `download-external-libs`.
* [dbg-macro](https://github.com/sharkdp/dbg-macro) — a lightweight `dbg(x)`
  macro, downloaded by `download-external-libs`.
* [glad](https://github.com/Dav1dde/glad) — the OpenGL 4.5 loader, vendored
  under `src/GPU/Backends/GL45/glad/`.
* GLFW — the window and context toolkit used by the examples (system package).

The GLEW / SOIL / bullet3 / json dependencies of the previous design are no
longer used.

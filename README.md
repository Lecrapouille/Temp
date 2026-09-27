# Compages

A C++20 library for drawing and computing on the GPU without writing graphics-API
calls. The `gpu::` layer is the foundation: OpenGL 4.5 core with Direct State
Access today, another backend tomorrow. Switching `GPU_BACKEND` in
`Makefile.common` is how a second backend would be selected; nothing under
`src/GPU/` except `src/GPU/Backends/` is allowed to include a `gl*` header.

Three layers, each usable without the next one:

- **`gpu::`, the shader first.** A `gpu::Drawable` is a program plus the data
  it reads, filled by the names the shader declares
  (`triangle["position"] = {...}`) or from an interleaved C++ struct. Uniforms
  and textures are set the same way, and everything is sent to the device at
  the next draw.
- **`scene::`, entities and components.** A headless `World` (EnTT inside)
  where entities are created and composed flecs-style,
  `world.entity("Ship").set(Velocity{}).child("Gun")`, and a `Scene` that
  shows that World with Three.js-style shortcuts: `scene.box("Crate",
  scene::texture("crate.jpg"))`, `scene.camera()`, `scene.sun()`,
  `scene.load("Duck.glb")`.
- **Behaviors, Unity-style.** `struct Spin : scene::Behavior { void
  update(float dt) override; }` attached with `entity.add<Spin>(2.0f)`; it
  reaches its entity, its transform, the input and the frame.

```cpp
scene::World world;
scene::Scene scene(world);

scene.camera().position(0, 2, 6).add<scene::Orbit>();
scene.sun();
scene.box("Crate", scene::texture("wooden-crate.jpg")).add<Spin>(1.0f);

// every frame
scene.draw(frame);
```

Errors are values (`compages::Result<T>`, `compages::Status`) where they can
happen, at loading time. What goes wrong while drawing a frame is reported once,
with a sentence naming both sides of the mismatch, and handed back by
`Scene::prepare()` or `gpu::takeFrameError()`, so a frame does not need a
check after every line. See [doc/Architecture.md](doc/Architecture.md) for the
map of the layers and [doc/Design.md](doc/Design.md) for the *why*.

Physics (the former ReactPhysics3D adapter) is parked in `attic/Physics/`,
outside the build, until it is rewritten on top of the new `scene::` API.

## Quick start

The library never opens a window. GLFW, SDL or Qt does, then hands over a
function that resolves driver symbols:

```cpp
#include <Compages/Compages.hpp>

if (auto ready = gpu::init(glfwGetProcAddress); !ready)
{
    std::cerr << ready.error() << std::endl;
    return EXIT_FAILURE;
}

// ...

gpu::shutdown();
```

The smallest picture is a shader and its data, filled by the names the shader
declares:

```cpp
gpu::Drawable triangle;
COMPAGES_TRY(triangle.load(vertex_src, fragment_src));
triangle["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };
triangle["color"]    = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

// every frame, inside the pass the window opened
gpu::clear({ 0.1f, 0.1f, 0.15f });
triangle.draw();
```

A misspelled attribute or a `vec2` given three numbers is reported with the
list of what the shader really declares. The lower pieces (`Buffer<T>`,
`Pipeline`, `draw`) are still there for the cases the Drawable does not cover.

Build:

```
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make download-external-libs
make compile-external-libs
make -j8 all             # the library and gallery
make -j8 -C tests all && ./build/Compages-UnitTest
make -j8 -C examples all && ./build/Compages-examples
```

`./build/Compages-examples --check` builds, draws and closes every
example, and fails if any of them left a handle behind. That is the leak test
as much as the smoke test. `make -C examples check-contracts` is the
standard-library-only static gate for the authoritative manifest, old-demo
parity and documented logic budgets; building the gallery runs it automatically.

More on building: [doc/Install.md](doc/Install.md). What each example is for:
[examples/README.md](examples/README.md).

## Why OpenGL 4.5

3.3 gives VAOs, instancing and uniform blocks, but no compute and no shader
storage. Without those, every GPU calculation has to be a ping-pong through a
texture: one texel, one thread, no random write. That is examples 10 and 11,
and it is what the previous design of this library had.

4.3 (2012) is the real step: compute shaders, SSBO, debug output. 4.5 (2014)
adds Direct State Access, which removes the pattern of binding an object just
to configure it. That pattern was most of the fragile call-order code the old
`GLVAO` / `GLProgram` layer was made of.

Apple froze OpenGL at 4.1 and later deprecated it. There is no honest GL 4.5
backend for macOS. The way onto a Mac is a Vulkan backend through MoltenVK,
or Metal, not a second OpenGL backend that pretends 4.1 is enough. The backend
contract exists now so that day does not rewrite a line of `gpu::`.

The longer version of these choices, and of the three ways to compute on a GPU,
is [doc/Design.md](doc/Design.md).

## What is here

The `gpu::` layer:

| Piece | What it does |
|---|---|
| `Drawable` | A program and the data it reads, by attribute name or from an interleaved struct; sent at the next draw |
| `Buffer<T>` | Device memory; ergonomic `Buffer::from(range)` has a CPU mirror, dirty ranges and explicit/lazy upload |
| `Shader`, `Program` | Compilation and a full reflection of attributes, blocks and samplers |
| `Pipeline` | Program + C++ layout + render state, checked against each other once |
| `Texture`, `Framebuffer` | Images, and a target that is not the window |
| `UniformBlock` / `TypedUniformBlock<T>` | Shared uniforms at the offsets the driver chose, or at std140 |
| `ComputeProgram`, `PingPong<T>`, `barrier` | Work that is not a picture |
| `draw`, `drawIndexed`, `drawInstanced`, `drawIndirect` | Asking the device to draw |
| `Result<T>`, `Status`, `COMPAGES_TRY`, `reportError` | Failures come back as a sentence that names both sides |

The `scene::` layer on top:

| Piece | What it does |
|---|---|
| `World`, `Entity` | The simulation, flecs-style: `entity()`, `set<T>()`, `get<T>()`, `child()`, `lookup("A/B")`, `each<T...>()`. Runs without a device. |
| `Camera`, `DirectionalLight`, `PointLight`, `MeshRenderer` | Pure data hung on entities |
| `Behavior`, `Orbit`, `Fly` | Code hung on entities: `start()` once, `update(dt)` every frame |
| `Scene` | Shows a World: shapes and looks, cameras, lights, glTF loading, prefabs, skybox, picking, debug lines, `draw(frame)` |
| `AssetManager`, `Prefab`, `MeshAsset`, `Material` | Resources, shared between entities and loaded separately from them |
| `SceneExtractor`, `RenderSnapshot`, `Renderer` | Snapshot the World, sort, draw |

The example gallery walks from a clear to a hundred thousand instanced sprites,
a GPU cull whose count the CPU never reads, and small scenes written with
behaviors. The older `GLVAO` / `GLProgram` demos sit in `examples/legacy/` and
are not compiled.

## Licence

GNU General Public License v3. See the header of any source file.
[Credits](doc/Credits.md).

# OpenGLCppWrapper

A C++20 library for drawing and computing on the GPU without writing graphics-API
calls. The `gpu::` layer is the foundation: OpenGL 4.5 core with Direct State
Access today, another backend tomorrow. Switching `GPU_BACKEND` in
`Makefile.common` is how a second backend would be selected; nothing under
`src/GPU/` except `src/GPU/Backends/` is allowed to include a `gl*` header.

On top of `gpu::` sits a small simulation and rendering stack:

- `assets::` — shared `MeshAsset`, `Material`, `MaterialInstance`, owned by an
  `AssetManager` and named by typed generational ids.
- `world::` — an Entity/Component simulation with a `SpatialGraph`, a SoA
  `TransformStore`, sparse-set `ComponentStore<T>`, pure-data components
  (`Camera`, `Light`, `MeshRenderer`). Nothing in `world::` touches `gpu::`;
  the whole layer runs headless.
- `scene::` — a *view* on a World: which camera Entity, which clear colour,
  which environment. Two Scenes can share one World.
- `render::` — `Extractor::extract(scene) -> RenderSnapshot`, then
  `Renderer::render(pass, snapshot, assets)`. The snapshot is a value; the
  renderer never reaches back into the World.

This is not a game engine. No GameObject, no virtual `onDraw`, no animation, no
physics, no input, no file loader. Behaviours are added by writing over
components; a glTF or OBJ loader would deposit its results in the AssetManager
and hand back ids. See [doc/Architecture.md](doc/Architecture.md) for the map
of the layers, [doc/Design.md](doc/Design.md) for the *why*.

## Quick start

The library never opens a window. GLFW, SDL or Qt does, then hands over a
function that resolves driver symbols:

```cpp
#include "GPU/GPU.hpp"

if (auto ready = gpu::init(glfwGetProcAddress); !ready)
{
    std::cerr << ready.error() << std::endl;
    return EXIT_FAILURE;
}

// ...

gpu::shutdown();
```

A frame is a pass, a pipeline and a draw. The pipeline is where the C++ vertex
layout is checked against the shader, once, at load time:

```cpp
struct Vertex { Vector2f position; Vector3f color; };
static const gpu::VertexLayout LAYOUT = GPU_LAYOUT(Vertex, position, color);

auto program = gpu::Program::fromSources(vertex_src, fragment_src);
auto pipeline = gpu::Pipeline::create<Vertex>(program.value(), LAYOUT);
auto vertices = gpu::Buffer<Vertex>::from(corners,
                                          gpu::BufferKind::Vertex,
                                          gpu::BufferUsage::Immutable);

auto pass = gpu::RenderPass::begin({ .width = w, .height = h });
gpu::Status drawn = gpu::draw(pipeline.value(), vertices.value());
```

Every call that can fail returns `gpu::Result<T>` or `gpu::Status`. `GPU_TRY`
forwards the sentence the library wrote, which names both sides of the mismatch
rather than a black screen.

Build:

```
git clone --recurse-submodules https://github.com/Lecrapouille/OpenGLCppWrapper.git
cd OpenGLCppWrapper
make download-external-libs
make compile-external-libs
make -j8                 # the library
make -j8 -C tests && ./build/OpenGLCppWrapper-UnitTest
make -j8 -C examples && ./build/OpenGLCppWrapper-examples
```

`./build/OpenGLCppWrapper-examples --check` builds, draws and closes every
example, and fails if any of them left a handle behind. That is the leak test
as much as the smoke test.

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
| `Buffer<T>`, `VertexArray<T>` | Memory on the device, with a CPU mirror and dirty ranges when vertices move |
| `Shader`, `Program` | Compilation and a full reflection of attributes, blocks and samplers |
| `Pipeline` | Program + C++ layout + render state, checked against each other once |
| `Texture`, `Framebuffer` | Images, and a target that is not the window |
| `UniformBlock` / `TypedUniformBlock<T>` | Shared uniforms at the offsets the driver chose, or at std140 |
| `ComputeProgram`, `PingPong<T>`, `barrier` | Work that is not a picture |
| `draw`, `drawIndexed`, `drawInstanced`, `drawIndirect` | Asking the device to draw |
| `Result<T>`, `Status`, `GPU_TRY` | Failures come back as a sentence that names both sides |

The simulation & rendering stack on top:

| Layer | Piece | What it does |
|---|---|---|
| `assets::` | `AssetManager`, `MeshAsset`, `Material`, `MaterialInstance` | Owns shared resources, addressed by typed generational ids |
| `world::`  | `World`, `Entity`, `SpatialGraph`, `TransformStore`, `ComponentStore<T>` | The simulation. Runs without a device. |
| `world::`  | Components: `Camera`, `DirectionalLight`, `PointLight`, `MeshRenderer` | Pure data hung on Entities |
| `scene::`  | `Scene`, `RenderSettings`, `Environment` | A view on a World |
| `render::` | `Extractor`, `RenderSnapshot`, `RenderQueue`, `Renderer` | Snapshot the World, sort, draw |

Seventeen examples walk from a clear to a hundred thousand instanced sprites,
a GPU cull whose count the CPU never reads, and three moving robots that share
one mesh through the whole `world::`/`scene::`/`render::` pipeline. The older
`GLVAO` / `GLProgram` demos sit in `examples/legacy/` and are not compiled.

What is not here, and not started: an animation mixer, model loaders (glTF,
OBJ), a second backend, a physics system.

## Licence

GNU General Public License v3. See the header of any source file.
[Credits](doc/Credits.md).

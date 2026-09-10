# OpenGLCppWrapper Architecture

This document is the map of the codebase: what each layer is, what it is
allowed to know about the one under it, and what the previous attempts got
wrong. The companion [Design.md](Design.md) is the *why* — why this vertex
layout style, why 4.5, why handles and pools. This file is the *what and
where*.

The library is a stack of layers. Each layer only knows the ones below it. A
    10|layer never reaches up.

```
   examples/                     GLFW window, ImGui overlay
   ─────────────────────────────────────────────────────────
   render::   Extractor → RenderSnapshot → RenderQueue → Renderer
   scene::    Scene, RenderSettings, Environment
   world::    World, Entity, SpatialGraph, TransformStore, Components
   assets::   AssetManager, MeshAsset, Material, MaterialInstance
   ─────────────────────────────────────────────────────────
   gpu::      Buffer, VertexArray, Program, Pipeline, RenderPass,
    20|              Texture, Framebuffer, Compute, PingPong, barrier, …
   ─────────────────────────────────────────────────────────
   backend    src/GPU/Backends/GL45/   (only place a gl* symbol lives)
```

`gpu::` and everything above it is the product. The backend is one file
selected by `GPU_BACKEND` in `Makefile.common`.

## The layers, from the ground up

### The backend
    30|
Directory: `src/GPU/Backends/GL45/`.

Talks to one graphics API. Today: OpenGL 4.5 core with Direct State Access
(DSA). It receives native ids, sizes, and our own enums; it never sees a
`gpu::Buffer`. It is the only place a `gl*` symbol may appear. A second
backend (Vulkan/Metal) would sit next to it, and `gpu::` would not change.

### `gpu::`

Directory: `src/GPU/`.
    40|
The C++ face of the device. Nothing in it exposes a `GLenum`. It offers:

| Type | Role |
|---|---|
| `Buffer<T>`, `VertexArray<T>` | Device memory with a CPU mirror and dirty ranges |
| `Shader`, `Program` | Compilation and full reflection (attributes, blocks, samplers) |
| `Pipeline` | Program + C++ layout + render state, checked once at load time |
| `Texture`, `Framebuffer` | Images and off-screen targets |
| `UniformBlock` / `TypedUniformBlock<T>` | Shared uniforms at driver or `std140` offsets |
    50|| `ComputeProgram`, `PingPong<T>`, `barrier` | Work that is not a picture |
| `RenderPass` | A framed viewport with a clear |
| `draw`, `drawIndexed`, `drawInstanced`, `drawIndirect` | Drawing calls |
| `Result<T>`, `Status` | Explicit error propagation, `GPU_TRY` for forwarding |

Handles are indexed generational references into pools. Destroying an object
twice is nothing; using a stale handle is a sentence, not a crash. Buffers
released by the caller shrink the pool's counters back to zero. The gallery's
resource counters are how that rule is watched.

    60|`gpu::` knows nothing about entities, scenes, transforms or lights.

### `assets::`

Directory: `src/Assets/`.

Owns the shared, reusable stuff — the things a component only wants to *name*.

| Type | Role |
|---|---|
| `MeshAsset` | GPU buffers of vertices + indices, plus a CPU bounds box |
| `Material` | A `gpu::Program` + a `gpu::Pipeline`, shared across instances |
    70|| `MaterialInstance` | A `MaterialId` + per-object parameters (colour, …) |
| `AssetManager` | Pools of the above, addressed by typed generational `AssetId`s |
| `Primitives.hpp` | `makeCube()`, `makeLitMaterial()` — the seeds an example needs |

An `AssetId` is a compact `{index, generation}` pair. `MeshAssetId`,
`MaterialId` and `MaterialInstanceId` are distinct types, so a mesh id cannot
be passed where a material is expected.

The AssetManager does not know which entities use an asset. It is where a
future glTF loader would deposit its results.
    80|
### `world::`

Directory: `src/World/`.

The simulation — the source of truth of *what exists*. It never touches
`gpu::`. A World can run headless (a test, a tool, a server).

| Type | Role |
|---|---|
| `Entity` | An `{index, generation}` handle. No data. Cheap to copy. |
| `EntityRegistry` | Hands out and reclaims Entity slots with a free list |
    90|| `SpatialGraph` | Parent / child / sibling links. Cycles refused. `KeepLocal`/`KeepWorld` reparenting. |
| `TransformStore` | SoA of local (position, rotation, scale) and derived world matrices, with a dirty bit per node |
| `TransformSystem` | Walks the graph parents-before-children and refills world matrices |
| `ComponentStore<T>` | Sparse set: O(1) add / remove / get, dense iteration over the components that exist |
| `OrbitController`, `FlyController`, `FPSController` | Systems that write a Camera entity's local transform from a `CameraInput` |
| `raycast` | Closest MeshRenderer AABB along a world `Ray` (bounds supplied by the caller) |
| `World` | Composes all of the above, plus a store per user component type, plus name and enabled flags |

Components in `World/Components/` are pure data:

| Type | Role |
|---|---|
   100|| `Camera` | Projection kind, fov, near/far — no view matrix, that comes from the Entity's transform |
| `DirectionalLight`, `PointLight` | Colour, intensity, range |
| `MeshRenderer` | A `MeshAssetId` + a `MaterialInstanceId` + `RenderFlags` |

No component holds a `gpu::` handle. No component holds an `Entity` that
would create ownership. Entities own components; components name assets.

### `scene::`

Directory: `src/Scene/`.

A *view* on a World. A Scene does not own the World or the AssetManager: it
   110|points at them, adds a camera Entity, a clear colour, an environment, and
that is it. Two Scenes can share the same World with different cameras
(player, minimap, editor preview, off-screen capture).

| Type | Role |
|---|---|
| `Scene` | `World&` + `AssetManager&` + active `Entity` camera + settings + env |
| `RenderSettings` | Clear colour, culling flag |
| `Environment` | Ambient colour, fallback light direction |

### `render::`

   120|Directory: `src/Render/`.

The bridge from simulation to picture. It is split in three so each part can
be tested and, later, moved to another thread.

| Type | Role |
|---|---|
| `CameraFrame` | An immutable snapshot of a camera: view, projection, inverses, frustum, `screenRay()` |
| `pick` / `pickAt` | Geometric pick: unproject a pixel, test MeshRenderer AABBs |
| `RenderSnapshot` | A frozen list of `RenderItem` + lights + camera + environment — no pointer back into the World |
| `RenderItem` | One thing to draw: `{entity, mesh id, material instance id, world matrix, world bounds, flags}` |
| `Extractor` | Reads a Scene → World → components, culls against the frustum, writes a snapshot |
   130|| `RenderQueue` | Sorts the snapshot's items by `(material, mesh)` to minimise state changes |
| `Renderer` | Reads a snapshot, walks the queue, emits `gpu::` calls |

The Extractor is where the World stops and rendering starts. Between an
extraction and the corresponding `Renderer::render`, the World may already be
running the next simulation step: the snapshot is what the frame draws.

## The frame, end to end

```
   1.  app       world.update()                         // TransformSystem
       app       Extractor::extract(scene, w, h)        // → RenderSnapshot
   140|   2.  app       RenderPass::begin({w, h, clear})
       app       renderer.render(pass, snapshot, assets)
   3.  Renderer  RenderQueue::build(snapshot)           // sort by (mat, mesh)
       Renderer  for each entry:
                     assets.material(mi.material).use()
                     program.set("model",  item.world_matrix)
                     program.set("view",   snapshot.camera.view)
                     program.set("proj",   snapshot.camera.projection)
                     program.set("color",  mi.parameters.color)
                     program.set("lightDir", light.direction)
   150|                 gpu::drawIndexed(pipeline, vertices, indices)
   4.  app       (end of pass, next frame)
```

The World never appears past step 2. The Renderer never appears before it.
That is the shape the roadmap in [Gloop_Architecture_Corrige.md](../Gloop_Architecture_Corrige.md)
targets, and the shape the tests validate: `WorldDrawTest.RendersACubeThroughTheWholePipeline`
starts from an empty World and ends inside a `gpu::` draw call.

## What each rule buys

- **No `gpu::` in `world::`.** The simulation can run in a test with no
   160|  device. `EntityRegistryTests`, `SpatialGraphTests`, `TransformTests` and
  `HierarchyTests` compile and run without a window.
- **No `world::Entity` in a component's semantics.** Components are pure data
  and can be relocated by a store without rewriting references.
- **Assets held by id, not by pointer.** A component that outlives an asset
  becomes a broken id, which the Extractor and Renderer detect and skip. No
  dangling pointer.
- **The snapshot is a value.** `Renderer::render` takes a snapshot, not a
  scene. That is what lets a future version put extraction on one thread and
  rendering on another, and what lets the renderer be tested against a hand-
   170|  built snapshot with no World at all.
- **Sorting on the renderer's side.** The application does not have to
  submit meshes in any order; the queue groups by material first, mesh
  second, so a hundred renderers of the same cube become one bind and one
  draw per material change.

## What is *not* here

- **No GameObject and no virtual `onDraw`.** Behaviours are added by a caller
  writing over components; the renderer never calls into user code.
- **No animation, no skinning, no physics, no input.** `TransformSystem` is a
   180|  stateless walk; there is nothing else on the update loop.
- **No file loader.** `Primitives.hpp` builds the cube in code. A glTF or OBJ
  importer would deposit its results in the AssetManager and hand back ids.
- **No shader library.** A `Material` carries its own `gpu::Program`. A
  library that produces shaders from a description is a layer above.
- **No second backend, yet.** The contract exists so it can be added without
  rewriting a line of `gpu::`. See [Design.md](Design.md) on macOS / mobile.

## Where the pieces sit in the tree

```
   190|src/
     Common/          # small utilities used everywhere (Exception, File, Path)
     Math/            # Vector, Matrix, Quaternion, AABB, Frustum, Units
     GPU/             # gpu:: layer
       Backends/      # one folder per backend; only GL45 today
       Core/          # types shared by every backend (Result, Layout, …)
       *.cpp/*.hpp    # Buffer, Program, Pipeline, RenderPass, …
     Assets/          # assets:: layer (AssetManager, MeshAsset, Material, Primitives)
     World/           # world:: layer
       Components/    # Camera, Light, MeshRenderer
   200|     Assets/           # assets:: layer (AssetManager, MeshAsset, Material, Primitives)
     Scene/           # scene:: layer (Scene, RenderSettings, Environment)
     Render/          # render:: layer (Extractor, RenderQueue, Renderer, snapshot types)

  examples/
    00_Basics/       # 01..08  — pass, triangle, cube, texture, framebuffer
    01_Scientific/   # 09..12  — heightmap, ping-pong textures, growing line
    02_Compute/      # 13, 14  — compute + storage buffers, N-body
    03_Performance/  # 15, 16      — sprite batch, indirect draw
    04_World/        # 17..20  — MovingRobot, ManyCubes, SplitViews, CameraPick
   210|    Common/          # Window (GLFW), Gallery (ImGui overlay)
    legacy/          # older GLVAO / GLProgram demos, not compiled

  tests/
    GPU/             # backend and gpu:: layer
    Math/            # linear algebra, AABB, Ray, Frustum, Units
    World/           # EntityRegistry, SpatialGraph, TransformStore, ComponentStore, Camera, Controllers, Raycast, Hierarchy, Transform, WorldDraw (full pipeline)
```

## Why the previous `src/Scene` was retired

   220|The old `src/Scene` was a tree of nodes with virtual `onDraw` callbacks. Each
node held its own `gpu::` handles: a mesh node owned a VBO, a light node owned
a uniform block, a camera node owned a matrix. Reparenting a node meant
re-uploading a matrix. Sharing a mesh between two nodes meant refcounting
buffers by hand. Drawing was a depth-first walk that called into user code at
every node.

That design conflated four things:

1. What exists in the simulation (**World**).
   230|2. Who is under whom (**SpatialGraph**, now a distinct system inside the
   World).
3. What is presented, from where (**Scene**, now a plain view context).
4. How pixels get on the screen (**Renderer**, now consuming a snapshot).

The current split gives each concern one file to read, and one interface to
test. `src/Scene/` is now the *view* layer, twenty lines wide, and the old
tree of `SceneNode` is gone. Three examples exercise the stack:
`17_MovingRobot` is the smallest one, three robots of cubes sharing one
mesh and one material; `18_ManyCubes` scales that to close to two thousand
entities to show what the material sort buys; `19_SplitViews` renders the
same World twice per frame from two Scenes, a perspective camera on the
left and an orthographic top-down camera on the right; `20_CameraPick`
navigates with Orbit/Fly/FPS controllers and selects a cube with a
geometric raycast.
   240|
## Rules of thumb

- If a class in `world::` needs a `gpu::` type, the class is in the wrong
  layer. Move it to `assets::` or produce it in the Renderer from the
  snapshot.
- If a component holds a pointer to another component or to an Entity, it is
  either a system's job (walk the graph) or the wrong data (store an id and
  resolve it lazily).
- If the renderer reaches into the World, extract that state into the
  snapshot instead.
   250|- If two Scenes want the same picture with a different camera, the code
  supports it already; do not copy the World.
- If a test needs to open a window to prove a simulation is correct, the
  test is exercising the wrong layer.

The road ahead — instanced rendering by material, render passes for shadows,
a UI/2D layer, an asset loader — is what the numbered stages of
[Gloop_Architecture_Corrige.md](../Gloop_Architecture_Corrige.md) address.
This document describes what is compiled today.

# Design of the `gpu::` layer

This document is why the library looks the way it does. The public headers say
what each type is for; this says which other shapes were considered and why
they lost.

Nothing here exposes a `GLenum`. The day a second backend exists, this file
should still be true.

## Layers

Six things sit on top of each other, and each one is allowed to know only
the ones below it.

**The backend** talks to one graphics API. Today that is OpenGL 4.5 core with
Direct State Access, in `src/GPU/Backends/GL45/`. It receives native ids,
counts and our own enums. It never sees a `gpu::Buffer`. It is the only place
a `gl*` symbol may appear. `Makefile.common` selects it with `GPU_BACKEND`.

**The `gpu::` layer** is the product. Handles, pools, `Result`, layouts,
buffers, programs, pipelines, passes, compute. A caller writes C++ and GLSL
and never an attribute location. Failures come back as a sentence that names
both sides of the mismatch.

**The `assets::` layer** owns the shared stuff — a `MeshAsset` (GPU buffers
+ bounds), a `Material` (program + pipeline), a `MaterialInstance` (parameters
for one draw). Everything is addressed by a typed generational id. Components
hold ids, not pointers, so a mesh can be freed independently of the entities
that used to draw it.

**The `world::` layer** is the simulation. Entities are compact
`{index, generation}` handles with no data. A `SpatialGraph` holds parent /
child / sibling links. A `TransformStore` holds local (position, rotation,
scale) and derived world matrices in SoA arrays with dirty bits. Components
(`Camera`, `DirectionalLight`, `PointLight`, `MeshRenderer`) are pure data
hung on entities through `ComponentStore<T>`. Nothing in `world::` ever
touches `gpu::`; the whole layer can run headless.

**The `scene::` layer** is a *view* on a World: which camera Entity is used,
which clear colour, which environment. It does not own the World or the
AssetManager, it references them. Two Scenes can share one World with two
cameras.

**The `render::` layer** is the bridge. `Extractor::extract` reads a Scene,
walks the World's components, culls against the camera frustum, and returns
a `RenderSnapshot` — a value that stands on its own with no pointer back into
the World. A `Renderer` consumes a snapshot, sorts the items by material,
then mesh, and emits `gpu::` calls. Between an extraction and its render the
World may already be running the next simulation step; the snapshot is what
the frame draws.

Building `world::` before the layer below it could refuse a bad layout is how
the previous `src/Scene` (a tree of `SceneNode` with virtual `onDraw`) stalled.
That folder is gone; the small `src/Scene/` today is only the view context.

The window is not a layer. GLFW lives in `examples/Common/Window.cpp` and
nowhere else. `gpu::init` is handed a loader (`glfwGetProcAddress`,
`SDL_GL_GetProcAddress`, …) and a context that already exists. Tests use an
invisible window for the same reason.

## Why the C++ layout decides, and the shader confirms

The previous design derived the vertex layout from the shader. A program was
introspected, one buffer was created per attribute, and a VAO belonged to one
program forever: `GLProgram::bind(vao)` refused a VAO that had been used with
another program. A mesh could not be drawn twice with two shaders. A shadow
pass, a normals view and a wireframe were therefore three copies of the same
vertices.

That choice is excellent for scientific visualisation. A researcher writes a
shader, fills the arrays the shader asked for, and draws. Glumpy works that
way, and examples 09 to 12 are in that spirit. It is the wrong default for
anything that draws the same mesh more than once.

Here the layout is declared in C++ (`gpu::field`, `gpu::describe`, or
`GPU_LAYOUT` when the names already match). The shader is only asked to
confirm that it can be fed by it. Fields the shader does not read are allowed
and ignored. Example `06_MultiPassMesh` is the proof: one `Buffer<Vertex>`
feeds three pipelines, and the overlay's vertex-reader count is one, not
three, because the three ways of reading happen to be three subsets of the
same stride.

The confirmation is a real one. Offering `aPosition` to a shader that
declares `position` used to draw nothing, silently. It is now an error that
prints both lists. Same for a `vec2` field feeding a `vec3` attribute.

bgfx, sokol_gfx and regl all landed on the same side of this choice. The
automatic binding of glumpy is still available as a writing style: name the
C++ fields the way the shader does, and `GPU_LAYOUT` is one line. The
difference is that the line is on the C++ side, so a second shader can share
the buffer.

## Handles and pools, not RAII of native objects

A `gpu::Buffer<T>` still releases its memory when it is destroyed. That is
ownership of a handle, not ownership of a driver object with a destructor that
calls `glDelete*`.

The driver objects live in pools, addressed by an index and a generation. A
stale handle is a number that no longer names anything, and every public call
checks the generation before it touches the pool. Destroying twice is
nothing. Using after destroy is a sentence, not a crash.

Two things this buys:

- **A lost context.** Recreating the device means emptying the pools and
  making the objects again. Callers still hold the same handles only if they
  rebuild; they do not hold raw ids that the driver has reused for something
  else.
- **Locality.** The records the backend walks sit next to each other. A
  `std::unique_ptr` per buffer would scatter them, and the previous layer did.

What it costs is that a handle is not a resource. Copying a `Buffer<T>` is
forbidden on purpose; copying a `BufferHandle` is not, and the copy does not
keep the memory alive. The typed wrappers are the owners. The gallery's
resource counters are how that rule is watched: closing an example has to
bring them back to zero.

## What each OpenGL version actually adds

Dates are the core specifications, not the year a given driver shipped them.

| Version | Year | What we would use |
|---|---|---|
| 3.3 | 2010 | VAO, instancing, uniform blocks, `GL_PROGRAM_POINT_SIZE` as an extension of habit |
| 4.1 | 2010 | Separate shader objects. Nothing this library needs. |
| 4.2 | 2011 | Immutable texture storage, atomic counters |
| **4.3** | **2012** | **Compute shaders, SSBO, `KHR_debug`, indirect draw** |
| 4.4 | 2013 | Persistent mapped buffers |
| **4.5** | **2014** | **Direct State Access** |
| 4.6 | 2017 | SPIR-V, anisotropic filtering in core |

**4.3 is the real step.** Without compute and SSBO, every calculation on the
device is a fragment shader writing a texture. That works. It is also one
texel, one thread, and no write except the pixel the fragment covers. A
galaxy of stars pulling on each other can be done that way. It is much more
contorted than a storage buffer and a work group.

**4.5 is the quality of the code.** DSA means `glNamedBufferStorage` instead
of bind-the-buffer-then-upload, and `glVertexArrayVertexBuffer` instead of
bind-the-VAO-then-bind-the-VBO. The previous layer was mostly that
call-order. Removing it is why a pipeline can share a way of reading a vertex
across programs, and why a draw binds the buffer at draw time rather than
baking it into a VAO that then refuses to change.

4.6 is what glad is generated against, so debug output and the rest of core
are there. The API is written to 4.5. SPIR-V is not consumed.

## Why not stay on 3.3

A 3.3 library would compile on more machines, including a 2011 Mac. It would
also have no honest `ComputeProgram`, no `PingPong<T>` of storage buffers, and
no `drawIndirect` filled by a shader. Examples 13 to 16 would have to be
rewritten as ping-pong textures, and the point of those examples is that they
are *not* that.

The scientific half of the gallery (09–12) would survive. The compute half
would become a lie or a second, slower path. One path that needs 4.3 is
clearer than two paths that pretend they are the same API.

## macOS and the 4.1 ceiling

Apple shipped OpenGL 4.1 and stopped. macOS 10.14 deprecated the API. There
is no compute shader, no SSBO, no DSA, no `KHR_debug` on that path.

A GL 4.1 backend would compile. It would not run `14_Galaxy` as written. The
honest ports are:

- rewrite the compute examples as ping-pong textures, and accept that
  instancing and uniform blocks are all 3.3/4.1 really offer, or
- add `Backends/Vulkan` and talk to Metal through MoltenVK.

The second is the one the backend contract is for. The first is a demo of
what 4.1 cannot do, not a product. The macOS job in CI still tries to compile
the current tree; it cannot be a runtime test of compute until a second
backend exists.

## Mobile, Raspberry Pi, the web

GL ES 3.1 (2014) has compute shaders and SSBO. A backend targeting ES 3.1
would run on recent phones and on a Pi 4 or 5. It would not have DSA: every
configuration would go back to bind-to-edit, hidden in that backend. Worth
doing when someone needs those machines; not a reason to drop DSA on desktop.

WebGL 2 is ES 3.0. No compute. The web target is WebGPU, which is a different
backend, not a dialect of this one.

## Three ways to compute on a GPU

They are not interchangeable. Each one has a constraint the next one removes.

### 1. Ping-pong through a texture

A fragment shader writes the pixel it covers and cannot read the picture it is
writing. The next state goes into a second texture, then the two swap.
Examples `10_GameOfLife` and `11_GrayScott`. Works on 3.3. One texel, one
thread. Random write is not available: a particle cannot move to another
pixel, it can only change the pixel it is.

This is the historical glumpy answer to “can the GPU write a texture?”. Yes,
exactly one texel per invocation, and not the one being read.

### 2. Transform feedback

Available since 3.0. A vertex shader writes varyings into a buffer. There is
still no random access: vertex *i* writes record *i*. Useful for particle
systems that only need to update themselves. This library documents the
option and does not implement it. Compute does the same job and the jobs
transform feedback cannot do.

### 3. Compute and a storage buffer

Since 4.3. A shader runs because it was asked to, over a grid the shader
itself declares. It reads and writes any element of a `Buffer` created with
`BufferKind::Storage`. The same buffer is then drawn as vertices. There is no
copy and no conversion. Examples `13_ComputeParticles` and `14_Galaxy`.
`16_IndirectDraw` goes one step further: the compute pass also writes the
draw command, and the CPU never asks how many objects survived.

`gpu::barrier()` is what makes those writes visible. Forgetting it is a race,
not a compile error. The library does not insert a barrier inside
`dispatch()`: the caller knows what the next pass will read.

## Instancing, one binding, generated corners

The backend describes a vertex with a single buffer binding. If any field is
`perInstance()`, the whole binding advances once per instance. Mixing
per-vertex corners with per-instance colour in one layout is therefore not
something the library can say.

The shape that fits is the one `15_SpriteBatch` uses: the four corners of a
quad come from `gl_VertexID`, and the buffer holds only instance records. A
hundred thousand sprites are one `drawInstanced` of four vertices. A layout
that needs both rates would be a second binding, which is future work, not a
missing flag on `Field`.

`p_first` on `draw` is the other half of batching: several meshes can sit in
one buffer and be drawn by offset. There is no arena type. The offset is
enough.

## From a triangle to a galaxy

The examples are meant to be read in this order. Each file's header comment
is the part that says which idea and why.

| | Example | The idea |
|---|---|---|
| 01 | ClearScreen | A pass: where, and what it starts from |
| 02 | Triangle | Shader, vertex struct, buffer, pipeline |
| 03 | DynamicTriangle | Send only what changed |
| 04 | TexturedQuad | A sampler is a unit number |
| 05 | IndexedCube | Indices, depth, culling, three matrices |
| 06 | MultiPassMesh | One buffer, three pipelines, one uniform block |
| 07 | RenderToTexture | A framebuffer is a target of its own size |
| 08 | Mandelbrot | No vertex buffer at all |
| 09 | HeightMap | A hundred thousand vertices the CPU recomputes |
| 10 | GameOfLife | Ping-pong texture, one bit per cell |
| 11 | GrayScott | The same ping-pong, two floats per cell |
| 12 | Lorenz | A line that grows; only the new point travels |
| 13 | ComputeParticles | The same buffer, written then drawn |
| 14 | Galaxy | N-body in tiles of shared memory, `PingPong` |
| 15 | SpriteBatch | A hundred thousand sprites, one draw |
| 16 | IndirectDraw | The count itself is a GPU result |
| 17 | MovingRobot | A tree of transforms; the queue draws, nodes do not |

## What World is, and what it is not

`world::` is the simulation. An `Entity` is an `{index, generation}` handle
with no data attached to it, the same idea as a `gpu::Handle`. The World
composes four things that used to be one:

- `EntityRegistry` hands out and reclaims Entity slots with a free list.
- `SpatialGraph` holds the parent / child / sibling links; cycles are
  refused by `setParent`, and reparenting has a `KeepLocal` / `KeepWorld`
  policy.
- `TransformStore` holds local (position, rotation, scale) and derived world
  matrices in SoA arrays with a dirty bit per node.
- `ComponentStore<T>` is a sparse set: O(1) add / remove / get, dense
  iteration over the components that exist. There is one per component type
  the user asks for, plus first-class stores for name and enabled flag.

`World::update` runs `TransformSystem`, which walks the graph parents-before-
children and refills only the dirty world matrices. Nothing else runs on the
update loop.

Components in `World/Components/` are pure data. A `Camera` has no view
matrix — the view is derived from the entity's transform at extraction time.
A `MeshRenderer` holds a `MeshAssetId` and a `MaterialInstanceId`; it does
not point at a `gpu::Buffer`, or at the entity that owns it. A `Light` is a
colour and intensity, and its direction is again the entity's forward axis.
No component holds a `gpu::` handle. No component holds an `Entity` in a
way that would make it own that entity.

Drawing is not `World::draw`. The `world::` layer has no `render` method at
all. Presentation is a separate stack:

- `scene::Scene` picks *which* World and *which* camera Entity are used.
- `render::Extractor` writes a `RenderSnapshot`.
- `render::Renderer` consumes the snapshot and emits `gpu::` calls.

The snapshot is the interface. It is a value; it points at nothing in the
World. That is what lets the Renderer be tested against a hand-built
snapshot with no World at all, and what will let extraction and rendering
live on different threads.

`world::` is not a game engine. There is no GameObject, no `onDraw`, no
Behavior, no ShaderLib, no GLB, no prefab, no animation mixer, no physics,
no input. Assets, materials and mesh data live one layer down, in `assets::`,
owned by an `AssetManager` and named by ids. A file loader would deposit its
results there and hand back ids to the caller; nothing in `world::` would
change.

A material at this level is one shared `gpu::Pipeline` (owned by a
`Material`), plus per-instance parameters (owned by a `MaterialInstance`). A
mesh is a `gpu::Buffer` of positions and normals plus an AABB, held by a
`MeshAsset`. The example does not own either; the AssetManager does.
`17_MovingRobot` is the smallest exercise of the whole stack: three robots
share one cube mesh and one lit material, and the World never names a
`gpu::` type. `18_ManyCubes` scales that pattern to a couple of thousand
entities with several material instances, so the RenderQueue's material sort
becomes visible. `19_SplitViews` renders one World from two Scenes side by
side (perspective + top-down), so the separation of *world*, *scene* and
*render* pays off in cost, not just in tidiness. `20_CameraPick` is the
camera chapter: controllers write a Camera entity's transform, and a
pixel becomes a world `Ray` that picks a MeshRenderer AABB.

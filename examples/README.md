# The examples

One program holds all of them. Build it and run it:

```
cd OpenGLCppWrapper
make -j8                 # the library
make -j8 -C examples     # the gallery
./build/OpenGLCppWrapper-examples
```

Arrows move to the next or previous example, space hides the overlay, escape quits.
Passing a name starts there:

```
./build/OpenGLCppWrapper-examples 05_IndexedCube
```

## Why one program and not twenty

The overlay shows two sets of numbers: what the current frame cost, and what the
device is holding for the example being shown. Switching from one example to the
next inside a live device is what proves the resources of the one being left are
actually given back, and the second set of numbers is where a leak shows up, as a
count that does not come back down.

That is also a test rather than a demonstration, so it can be run without a person
watching:

```
./build/OpenGLCppWrapper-examples --check
```

Every example is built, drawn for a few frames and closed. The program fails, with a
list, if any of them could not run or left anything on the device. Add
`--shots doc/pictures` to write one picture per example, which is where the pictures
in the documentation come from and a stronger check than "it did not crash": a black
picture means the example drew nothing.

`--cycle` moves on by itself every few seconds, for leaving on a second screen while
working on the library.

## What each one is for

They are meant to be read in order. Each adds one idea to the one before, and says
in its own comments which idea and why. The header comment of each file is the part
worth reading first.

### 00_Basics

| Example | What it is about |
|---|---|
| `01_ClearScreen` | A pass: where in the window a frame is drawn and what it starts from. No resource at all. |
| `02_Triangle` | The four things it takes to draw anything: a shader, a vertex struct, a buffer, and a pipeline that checks the three agree. |
| `03_DynamicTriangle` | Vertices that move, with only what was written travelling to the device. |
| `04_TexturedQuad` | A texture read by a shader, a sampler set by the unit it is bound to, and a triangle strip. |
| `05_IndexedCube` | A solid: indices, depth testing, back face culling and the three matrices. |
| `06_MultiPassMesh` | One `Buffer<Vertex>` feeding three pipelines, and one uniform block they all read. |
| `07_RenderToTexture` | A framebuffer: the cube is drawn into textures, then those textures are shown, once as they are and once through a fullscreen effect. |
| `08_Mandelbrot` | No vertex buffer. A fragment shader and the mouse, which is the point the zoom keeps still. |
| `28_MultiTextureBlend` | Legacy `03_MultiTexturedSquare`: two textures blended on a plane with a mix uniform. |
| `29_PointSphere` | Legacy `06_IndexedSphere`: GL_POINTS with a circular sprite in the fragment shader. |
| `31_ComplexShader` | Legacy `11_ComplexShader`: ShaderFrog Universe Nursery starfield on a quad. |
| `32_PostProcess` | Legacy `13_PostProdFrameBuffer`: textured cube and floor into an FBO with depth, then a wavy fullscreen pass. |

### 01_Scientific

| Example | What it is about |
|---|---|
| `09_HeightMap` | A hundred thousand vertices whose height the CPU recomputes. `VertexArray::modify()` sends the whole surface; the triangles stay in a `Buffer`. |
| `10_GameOfLife` | Ping-pong render-to-texture: each cell is a pixel, and a shader cannot read the picture it is writing. |
| `11_GrayScott` | The same ping-pong, holding two concentrations as 32-bit floats rather than a bit per cell. |
| `12_Lorenz` | A line that grows. `push_back` sends only the new point. |
| `30_Terrain3D` | Legacy `08_TerrainTexture3D`: smoothed random height field coloured by a six-layer 3D texture. |

### 02_Compute

| Example | What it is about |
|---|---|
| `13_ComputeParticles` | The same `Buffer` is a storage block, then vertices. A barrier is what makes the writes visible. |
| `14_Galaxy` | N-body gravity in tiles of shared memory, `PingPong<Star>`, `std430`. |

### 03_Performance

| Example | What it is about |
|---|---|
| `15_SpriteBatch` | A hundred thousand sprites, one draw. Corners from `gl_VertexID`, a buffer of per-instance records, an atlas of sixteen tiles. |
| `16_IndirectDraw` | A compute pass keeps the points inside a circle and writes the `DrawIndirectCommand`. The CPU never asks how many survived. |

### 04_World

The full simulation stack: `world::` for the entities, `assets::` for the
shared meshes and materials, `scene::` for what a view configures, and
`render::` for extraction and drawing.

| Example | What it is about |
|---|---|
| `17_MovingRobot` | A `world::` tree: three robots of cubes, one shared mesh, local rotations rewritten each frame from `sin(total_time)`. The child follows the parent because the world matrices say so, not because a callback walked the tree. |
| `18_ManyCubes` | One mesh, one program, five material instances, close to two thousand entities. The `RenderQueue` sorts by material so cubes of the same colour draw together, and the `Extractor`'s frustum culling drops the ones off-screen as the camera orbits. |
| `19_SplitViews` | One `World`, two `Scene`s, two `RenderPass`es side by side. A perspective camera on the left, an orthographic top-down camera on the right, both looking at the same simulation. Adding a third view is one more `Scene` and one more pass. |
| `20_CameraPick` | Orbit / fly / FPS controllers write the Camera entity's transform. A left click unprojects a pixel (`CameraFrame::screenRay`) and picks the closest MeshRenderer AABB. Selection is a material swap. |
| `21_GltfModel` | `importGltf` loads a static GLB: meshes, embedded textures and node hierarchy become AssetManager entries and World entities. ShaderLib's PBR shader draws the albedo map. Needs `Duck.glb` in `external/OpenGLCppWrapper-data/`. |
| `22_TexturedSpheres` | `makeSphere()` builds a UV sphere primitive. `makePbrMaterial()` and `TextureAsset` show the code-side asset path: base colour factors, optional albedo maps, one shared PBR pipeline. |
| `23_PrefabAndSave` | `makeRobotPrefab()` bakes the robot hierarchy as a reusable asset. `world::instantiate()` spawns three copies; `scene::save()` writes the World to JSON with asset names instead of runtime ids. |
| `24_MvpDemo` | MVP stack: AABB physics (`physics::PhysicsWorld`), point lights + gamma in ShaderLib, `Behavior` callbacks, `DebugDraw` collider boxes, `scene::saveScene()` with presentation settings. |
| `25_MiscLookAt` | Port of three.js `misc_lookat.html`: a thousand stretched cubes track a sphere on a Lissajous path; the camera eases toward the mouse. Uses `world::lookAt()`. |
| `26_Skybox` | Six JPG faces from OpenGLCppWrapper-data become a cube map drawn from the inside; a lit cube spins at the centre. Adapted from the legacy `09_SkyBoxTextureCube`. |
| `27_TextureGallery` | A 3×3 grid of PBR spheres showing textures from the data repository (`grassFlowers.png`, `rocks.png`, `wooden-crate.jpg`, …). Missing files fall back to flat colours. |
| `33_GeometryShowcase` | All built-in primitives on a plane: cube, box, sphere, cone, cylinder, pyramid and tube, each with lit, PBR, depth or normals material. |
| `34_AnimatedModel` | A walk-cycle character: unit joints, cylinders and a sphere as the mesh, opposite-phase `sin(total)` on hips and shoulders, root walking a circle. Two walkers share the meshes. |
| `35_PhysicsSandbox` | `physics::PhysicsWorld` on its own: dynamic cubes, a static floor, a kinematic pusher, collision events that flash a material, respawn when a cube leaves the slab. |
| `36_GltfAnimation` | `importGltf` loads `Soldier.glb` (skins + Idle/Walk/Run). `AnimationSystem` samples the clip onto Mixamo joints and skins the mesh. Keys 1/2/3 switch clips. |

## Where the window comes from

`Common/Window.cpp` uses GLFW, and it is the only file in the project that does.
Nothing under `src/` knows what a window is: the library is handed a context that
already exists and a function to resolve driver symbols with, which is what lets it
be used with SDL, with Qt, or inside an application that already has a window of its
own.

`Common/Gallery.cpp` uses Dear ImGui for the overlay, in the same spirit. Because
ImGui talks to the same context with its own shaders and its own state, the gallery
calls `gpu::forgetRenderState()` after it has drawn: the library sends only the state
that changed since the last pipeline, and that bet is off once somebody else has
touched the context.

## The previous examples

`legacy/` holds the demos written against the older `GLVAO`/`GLProgram` API. They are
not compiled. They are kept as a list of what the new layer has to be able to express:
a skybox, a terrain from a 3D texture, lighting, post processing through a
framebuffer, a scene graph. Each one comes back as the feature it needs arrives.

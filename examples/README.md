# The examples

One program holds all of them. Build it and run it:

```
cd Compages
make -j8                 # the library
make -j8 -C examples     # the gallery
./build/Compages-examples
```

The window is laid out like an editor, every panel docked and movable (View >
Reset the layout puts them back):

- **Viewport**, in the middle: the example, drawn into a texture the size of the
  panel, so that no panel hides part of it. The mouse goes to the example while it
  is over the viewport, the keyboard while the viewport has focus.
- **Examples**, on the left: the list by chapter (the folders), a green dot on those
  that ran, red on those that failed, orange on those that leaked.
- **Inspector**: what the example demonstrates, what its last frame cost, what it
  holds on the device. **Source**: its `.cpp` and `.hpp`. **Debug**: pause, one
  frame at a time, speed, wireframe, vsync, stop or go on after an error, break
  into the debugger. **Try it**, under them: the buttons and sliders of the
  example, when it has some.
- **Console**, at the bottom: what the library and the driver said, each message
  once with a count, filterable by level and by example.

| Key | Does |
|-----|------|
| PgUp / PgDn | previous / next example (the arrows are left to the examples) |
| F1 | hide / show the panels |
| F2 | wireframe |
| F5 | restart the example (also after fixing a failure) |
| F6 / F7 | pause / one frame |
| F12, Shift+F12 | screenshot of the example, of the whole window, into `screenshots/` |
| Esc | give the mouse back to the panels when a game holds it, otherwise quit |

Passing a name starts there:

```
./build/Compages-examples 04_DepthAndTransforms
```

## Why one program and not twenty

The Inspector shows two sets of numbers: what the current frame cost, and what the
device is holding for the example being shown. Switching from one example to the
next inside a live device is what proves the resources of the one being left are
actually given back, and the second set of numbers is where a leak shows up, as a
count that does not come back down.

That is also a test rather than a demonstration, so it can be run without a person
watching:

```
./build/Compages-examples --check
make -C examples check-contracts
```

The static contract gate checks the active-source manifest, ordering, duplicate
labels, old-to-new parity and pedagogical line budgets. It is also a prerequisite
of `make -C examples`, and CI runs it explicitly. The runtime check first validates
the same manifest registration, then every example is built, drawn for a few
frames and closed. The program fails, with a list, if any of them could not run or
left anything on the device. Add
`--shots doc/pictures` to write one picture per example (`--shots-with-panels` for
the whole window), which is where the pictures
in the documentation come from and a stronger check than "it did not crash": a black
picture means the example drew nothing.

`--cycle` moves on by itself every few seconds, for leaving on a second screen while
working on the library.

## What each one is for

They are meant to be read in physical directory and filename order. Variants sharing
one concept keep separate source files (`a`, `b`, `c`) so no demonstration is lost.
The gallery label returned by `name()` is the filename stem, which gives the same
order at runtime.

### Numbered target grid

`Common/ExampleManifest.hpp` is the authoritative numbered grid. Each record owns
the runtime label, conceptual target, source path, former demos covered, optional
budget and measured functions. `main.cpp` consumes that X-macro directly, so
adding an example cannot require keeping a second registration list in sync.

For concepts with variants, `01b_Triangle` is the budgeted
`01_ClearAndTriangle` lesson and `03a_TexturedQuad` is the budgeted
`03_TexturesAndSampling` lesson. The other variants remain independently runnable
without pretending that their combined source fits one introductory budget.

### 00_GettingStarted

| Example | What it is about |
|---|---|
| `00a_Dummy` | The Gallery lifecycle with no persistent GPU resource. |
| `00b_CpuGpuSync` | A buffer edited on the CPU, then sent: bars show what the GPU holds, the pending range in orange. |
| `00c_Compute` | A compute shader changing a buffer on the GPU, read back with `download()`. |
| `01a_ClearScreen` | A render pass, viewport and clear colour, with no GPU resource. |
| `01b_Triangle` | A shader, vertex struct, buffer and checked pipeline. |
| `01c_InterleavedTriangle` | An immutable interleaved vertex buffer, uploaded once. |
| `02_DynamicGeometry` | Vertices that move and upload only their dirty range. |
| `03a_TexturedQuad` | One sampled texture on a triangle strip. |
| `03b_MultiTextureBlend` | A blend map choosing between three ground textures. |
| `04_DepthAndTransforms` | Indexed solid geometry, depth, culling and transform matrices. |
| `05a_MultiPassMesh` | One vertex buffer feeding multiple pipelines and a shared uniform block. |
| `05b_RenderToTexture` | Rendering a cube to textures, then presenting and filtering them. |
| `05c_PostProcess` | A textured scene rendered to an FBO, followed by a fullscreen effect. |
| `06a_Mandelbrot` | Procedural fragment shading and mouse-centred zoom, without a vertex buffer. |
| `06b_ComplexShader` | A complex ShaderFrog starfield shader on a quad. |
| `07_PointClouds` | Point rendering with circular sprites in the fragment shader. |

### 10_ScientificAndCompute

| Example | What it is about |
|---|---|
| `10a_HeightMap` | A CPU-updated height field with separate vertex and index storage. |
| `10b_Terrain3D` | A smoothed random height field coloured by a layered 3D texture. |
| `11a_GameOfLife` | Ping-pong render-to-texture for a cellular automaton. |
| `11b_GrayScott` | Ping-pong floating-point textures for reaction-diffusion. |
| `12_ComputeParticles` | One buffer used by a compute shader and then as vertices. |
| `13_Galaxy` | Tiled shared-memory N-body gravity with ping-pong storage. |
| `15_Lorenz` | A streamed curve where each new point extends the GPU buffer. |

### 20_Performance

| Example | What it is about |
|---|---|
| `20a_SpriteBatch` | One draw for one hundred thousand atlas sprites. |
| `20b_ManyCubes` | Nearly two thousand entities, material sorting and frustum culling. |
| `21_IndirectDraw` | Compute culling that writes the indirect draw command without a CPU count readback. |

### 30_WorldAndAssets

The `scene::` layer: a headless `World` of entities and components, a `Scene`
that shows it with Three.js-style shortcuts, and behaviors hung on entities.

| Example | What it is about |
|---|---|
| `30_HeadlessWorld` | Entities, components, `each<>()`, a behavior and `lookup()`, without a Scene or a GPU. |
| `31_MovingRobot` | Hierarchical transforms: three robots built with `child()`, walking by a behavior. |
| `32a_SplitViews` | One World, two cameras, two viewports: a perspective eye and an orthographic map. |
| `32b_CameraPick` | `Orbit` and `Fly` camera behaviors, and picking with `scene.pick()`. |
| `32c_MiscLookAt` | A thousand cones turning toward a point that follows the mouse, with `lookAt()`. |
| `33a_TexturedSpheres` | Spheres with a colour, a texture, and a tinted texture. |
| `33b_TextureGallery` | Boxes showing the repository textures, with a colour when one is missing. |
| `33c_GeometryShowcase` | Every built-in shape, lit, textured, as depth and as normals. |
| `34_GltfModel` | `scene.load("Duck.glb")`, then `frameAll()` places the camera and a sun. |
| `35_PrefabAndSave` | A prefab built by hand, instantiated three times, and the scene saved to JSON. |
| `36a_AnimatedModel` | Walkers animated by a behavior, sharing their meshes. |
| `36b_GltfAnimation` | An imported skinned glTF and its clips, chosen in the Inspector with `scene.play()`. |
| `37_Skybox` | A cube-map environment with `scene.skybox()` around a lit cube. |

Physics (the former `40_Physics` examples) is parked in `attic/Physics/`, out
of the build, until it is rewritten on the new `scene::` API.

### 50_Complete

| Example | What it is about |
|---|---|
| `50_ThreeJsLike` | A box, a camera and a sun in a few lines, Three.js-style. |
| `51_Behaviors` | Three cubes driven by small behaviors: spin, bob, grow while space is held. |
| `52_MvpDemo` | Several behaviors, a moving lamp, debug lines and a ground. |
| `53_DoomLike` | A two-level Doom-like: a captured mouse look, WASD or arrows, an animated glTF shotgun (`R` reloads), ammunition and health pickups, torches to switch with `F`, soldiers who shoot back and robots who punch, blood and impact decals, particles, explosive barrels, a HUD. |

The enforced budgets count logical statements and control flow, not formatting:
comments, licences, shader strings, type declarations, signatures and class
boilerplate are excluded. Helpers containing lesson logic remain in scope
(`TexturedQuad::makeTexture`, for example). The MVP ≤20 gate measures the
integrated `MvpDemo::draw` frame path; setup remains deliberately exhaustive.

## Where the window comes from

`Common/Window.cpp` uses GLFW, and it is the only file in the project that does.
Nothing under `src/` knows what a window is: the library is handed a context that
already exists and a function to resolve driver symbols with, which is what lets it
be used with SDL, with Qt, or inside an application that already has a window of its
own.

`Common/Gallery.cpp` and `Common/GalleryPanels.cpp` use Dear ImGui (docking
branch) for the panels, in the same spirit. The example does not know it is drawn
into a texture: a pass naming no target draws into the pass around it, and the
gallery opens that one over the texture of the Viewport panel. Because
ImGui talks to the same context with its own shaders and its own state, the gallery
calls `gpu::forgetRenderState()` after it has drawn: the library sends only the state
that changed since the last pipeline, and that bet is off once somebody else has
touched the context.

## The previous examples

`legacy/` holds the demos written against the older `GLVAO`/`GLProgram` API. They are
not compiled. They are kept as a list of what the new layer has to be able to express:
a skybox, a terrain from a 3D texture, lighting, post processing through a
framebuffer, a scene graph. Each one comes back as the feature it needs arrives.

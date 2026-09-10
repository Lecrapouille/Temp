# Credits

## Why another C++ wrapper for OpenGL?

The library was originally a C++ take on the Python
[Glumpy](https://github.com/glumpy/glumpy)/gloo API: shader-first, hide the
`glBindBuffer` / `glVertexAttribPointer` / `glUseProgram` order, let the CPU
push pending vertices to the GPU whenever they changed. That version had a
`GLProgram`, a `GLVAO`, a `GLVBO`, and a scene tree of `SceneNode` objects
with virtual `onDraw` callbacks. It served a scientific-visualisation use
    10|case ([SimTaDyn](https://github.com/Lecrapouille/SimTaDyn)) well: a shader
was written, an array of vertices was filled, a draw call happened.

Two things dictated the shape of the current library.

First, sharing one mesh across two shaders was fundamentally awkward in the
Glumpy shape. A VAO was bound to one program forever; drawing a cube twice
with two pipelines meant two copies of the same vertices. That is fine for
science, wrong for anything that draws the same geometry more than once.
[bgfx](https://github.com/bkaradzic/bgfx),
[sokol_gfx](https://github.com/floooh/sokol) and
    20|[regl](https://github.com/regl-project/regl) all made the opposite choice:
declare the vertex layout on the C++ side, and ask the shader to confirm it
can be fed by it. `gpu::` does the same, and `06_MultiPassMesh` is the proof
that one buffer feeds three pipelines.

Second, `SceneNode::onDraw` had grown into a tree of virtual callbacks that
each held their own `gpu::` handles: a mesh node owned a VBO, a light node
owned a uniform block, a camera node owned a matrix. Reparenting reshaped
data. Sharing a mesh meant refcounting buffers by hand. Everything that
should have been a value became a lifetime problem. The current split —
    30|**World** (what exists), **SpatialGraph** (who is under whom), **Scene**
(what is presented from where), **Renderer** (how a snapshot becomes pixels)
— is the answer to that pile of coupling. The design borrows from
[Bevy](https://github.com/bevyengine/bevy)'s ECS shape (sparse-set
component stores, entities as generational handles, extraction into a
render world) and from [Three.js](https://github.com/mrdoob/three.js)'s
declarative feel (a Scene has a camera and lights, and `renderer.render(scene)`
is one line). See [Design.md](Design.md) and [Architecture.md](Architecture.md)
for the full walk of that reasoning.

    40|## Inspiration

- OpenGL wrappers
  - [Glumpy](https://github.com/glumpy/glumpy) — where the original vertex
    layout / pending-data idea comes from. Kept as the mental model of
    examples 09–12.
  - [bgfx](https://github.com/bkaradzic/bgfx),
    [sokol_gfx](https://github.com/floooh/sokol),
    [regl](https://github.com/regl-project/regl) — for the C++/JS-side
    vertex layout that swings the responsibility back to the caller.
    50|- Simulation & rendering stacks
  - [Three.js](https://github.com/mrdoob/three.js) — for how small a scene
    graph is allowed to look.
  - [Bevy](https://github.com/bevyengine/bevy) — for the ECS shape used by
    `world::` (generational entities, sparse-set component stores,
    extraction into a snapshot).
  - [scg3](https://github.com/vahlers/scg3) — an OpenGL 3 / C++11 scene
    graph used for teaching computer graphics.

## Third-parties

    60|`make download-external-libs` fetches and pins the third-parties. The list
lives in [Install.md](Install.md).

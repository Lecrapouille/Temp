# Tutorial

The numbered examples under `examples/00_Basics/` .. `examples/04_World/` are
the real tutorial: each one adds one idea to the one before, and its header
comment says which idea. This file is a shorter, non-runnable walkthrough of
the two stacks the library gives you, so a reader knows where to open next.

- [Drawing a triangle with `gpu::` alone](#drawing-a-triangle-with-gpu-alone)
- [Building a scene with `world::` + `scene::` + `render::`](#building-a-scene)
- [Where to go next](#where-to-go-next)

    10|## Drawing a triangle with `gpu::` alone

The bottom of the stack is `gpu::`. A minimal frame is: one shader program,
one C++ vertex layout, one buffer of vertices, one pipeline, one pass, one
draw.

```cpp
#include "GPU/GPU.hpp"
#include "Math/Vector.hpp"

// 1. The vertex struct on the C++ side. The layout is declared once and the
    20|//    pipeline checks the shader matches it — fields the shader does not
//    read are allowed and ignored.
struct Vertex { Vector2f position; Vector3f color; };
static const gpu::VertexLayout LAYOUT = GPU_LAYOUT(Vertex, position, color);

constexpr const char* VS = R"(#version 450 core
in vec2 position;
in vec3 color;
out vec3 vColor;
void main() { vColor = color; gl_Position = vec4(position, 0.0, 1.0); }
    30|)";

constexpr const char* FS = R"(#version 450 core
in vec3 vColor;
out vec4 fColor;
void main() { fColor = vec4(vColor, 1.0); }
)";

const Vertex CORNERS[] = {
    { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    40|    { {  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
    { {  0.0f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
};

// 2. `gpu::init` gets a loader (`glfwGetProcAddress`, `SDL_GL_GetProcAddress`)
//    from a context somebody else opened.
GPU_TRY(gpu::init(&glfwGetProcAddress));

GPU_TRY_ASSIGN(program,  gpu::Program::fromSources(VS, FS));
GPU_TRY_ASSIGN(pipeline, gpu::Pipeline::create<Vertex>(program, LAYOUT));
    50|GPU_TRY_ASSIGN(vertices, gpu::Buffer<Vertex>::from(
    CORNERS, gpu::BufferKind::Vertex, gpu::BufferUsage::Immutable));

// 3. A frame. `RenderPass::begin` clears; `draw` submits.
gpu::PassDesc desc;
desc.width  = window_w;
desc.height = window_h;
desc.color  = Vector4f(0.05f, 0.08f, 0.20f, 1.0f);
GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
GPU_TRY(gpu::draw(pipeline, vertices));
    60|```

The full working code is [`examples/00_Basics/02_Triangle.cpp`](../../examples/00_Basics/02_Triangle.cpp).
Everything else in `gpu::` is more of the same shape: create typed handles at
init, submit draws or dispatches inside a pass.

## Building a scene

Once several meshes and several nodes are involved, the four upper layers do
the bookkeeping. The idea is: the World owns *what exists*; the Scene picks
*which view*; the Extractor freezes the world into a value; the Renderer
    70|draws that value.

```cpp
#include "Assets/AssetManager.hpp"
#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Render/Extractor.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
    80|#include "World/Components/MeshRenderer.hpp"
#include "World/World.hpp"

assets::AssetManager assets;
world::World         world;
scene::Scene         scene(world, assets);
render::Renderer     renderer;

// 1. Register shared resources. Components will hold their ids, not pointers.
GPU_TRY_ASSIGN(cube_mesh_data, assets::makeCube());
    90|GPU_TRY_ASSIGN(cube_mesh, assets.addMesh("cube", std::move(cube_mesh_data)));

GPU_TRY_ASSIGN(lit_data,     assets::makeLitMaterial());
GPU_TRY_ASSIGN(lit_material, assets.addMaterial("lit", std::move(lit_data)));

GPU_TRY_ASSIGN(red, assets.addMaterialInstance("red",
    { lit_material, Vector3f(0.8f, 0.2f, 0.2f) }));

// 2. Populate the World. Entities are cheap; components are pure data.
world::Entity root = world.create("Root");
   100|world::Entity cube = world.create("Cube");
world.transform(cube).position = Vector3f(0.0f, 0.0f, 0.0f);
world.add(cube, world::MeshRenderer{ cube_mesh, red });
GPU_TRY(world.setParent(cube, root));

world::Entity sun = world.create("Sun");
world.transform(sun).rotation =
    Quatf::fromAngleAxis(units::angle::radian_t(-1.05),
                         Vector3f(1.0f, 0.0f, 0.0f));
world.add(sun, world::DirectionalLight{ Vector3f(1.0f), 1.0f });
   110|
world::Entity cam = world.create("Camera");
world.transform(cam).position = Vector3f(0.0f, 1.0f, 5.0f);
world::Camera camera;
camera.fov_degrees = 60.0f;
camera.near_plane  = 0.1f;
camera.far_plane   = 100.0f;
world.add(cam, camera);

// 3. Point the Scene at that camera and pick a clear colour.
   120|scene.setActiveCamera(cam);
scene.renderSettings().clear_color = Vector4f(0.05f, 0.08f, 0.20f, 1.0f);

// 4. Each frame: update transforms, extract a snapshot, render the snapshot.
world.update();

GPU_TRY_ASSIGN(snapshot, render::Extractor::extract(scene, window_w, window_h));

gpu::PassDesc desc;
desc.width  = window_w;
   130|desc.height = window_h;
desc.color  = scene.renderSettings().clear_color;
GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
GPU_TRY(renderer.render(pass, snapshot, assets));
```

What matters here:

- `world.transform(cube).position = ...` marks the entity dirty; the next
  `world.update()` refreshes only what changed, parents before children.
- Parent-child relationships come from `setParent`. Cycles are refused; a
   140|  world matrix is a standard TRS composition, so a parent's scale is
  inherited by its children the way one would expect.
- The Renderer never reads the World. The `snapshot` is what it draws. Two
  Scenes with two cameras produce two snapshots and two renders from the same
  World.

The full working code is [`examples/04_World/17_MovingRobot.cpp`](../../examples/04_World/17_MovingRobot.cpp).
Three robots share one mesh and one material; the animation rewrites each
joint every frame from `sin(total_time)`, so nothing accumulates. Its
neighbours in the same folder scale the same pattern further:
[`18_ManyCubes`](../../examples/04_World/18_ManyCubes.cpp) with a couple of
thousand entities to make the material sort visible, and
[`19_SplitViews`](../../examples/04_World/19_SplitViews.cpp) with two
Scenes on the same World rendered side by side, and
[`20_CameraPick`](../../examples/04_World/20_CameraPick.cpp) for orbit /
fly / FPS navigation plus a click-to-pick raycast.

## Where to go next
   150|
- [Architecture.md](../Architecture.md) — the map of the layers and the rules.
- [Design.md](../Design.md) — the *why* of each choice.
- [`examples/README.md`](../../examples/README.md) — the guided tour, one
  example at a time.
- [Install.md](../Install.md) — how to build, install and link against the
  library.

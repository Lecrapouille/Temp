# Compages core tutorial

## The GPU layer: the shader first

The application owns the window and context. Compages only receives the
function used to resolve GPU symbols:

```cpp
#include <Compages/Compages.hpp>

COMPAGES_TRY(gpu::init(glfwGetProcAddress));
```

A `gpu::Drawable` is a shader and the data it reads. The names are the ones the
shader declares:

```cpp
gpu::Drawable triangle;
COMPAGES_TRY(triangle.load(vertex_source, fragment_source));
triangle["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };
triangle["color"]    = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

// every frame
gpu::clear({ 0.1f, 0.1f, 0.15f });
triangle["time"] = total;   // a uniform, by the same syntax
triangle.draw();            // sends what changed, then draws
```

The same vertices can come interleaved from a struct whose fields are named
after the attributes. The field names are found by the compiler, with no macro
and no list to keep in sync:

```cpp
struct Vertex
{
    Vector2f position;
    Vector3f color;
};

triangle.vertices<Vertex>({ { { -0.8f, -0.6f }, { 1, 0, 0 } },
                            { {  0.8f, -0.6f }, { 0, 1, 0 } },
                            { {  0.0f,  0.8f }, { 0, 0, 1 } } });
```

A misspelled name or a `vec2` given three numbers is reported once, with what
the shader really declares, and `gpu::takeFrameError()` hands it back. What
the Drawable does not cover stays public: `Buffer<T>` with its CPU mirror,
`Pipeline`, `RenderPass`, `draw`, `drawIndirect`, compute.

## The World: entities and components

`scene::World` wraps an EnTT registry. It never touches the device.

```cpp
struct Velocity { Vector3f value; };

scene::World world;
scene::Entity ship = world.entity("Ship").set(Velocity{ { 1, 0, 0 } });
ship.child("Gun").position(0.0f, 0.5f, 0.0f);

world.each<Velocity>([&](scene::Entity e, Velocity& v) {
    e.position(e.position() + v.value * dt);
});

scene::Entity gun = world.lookup("Ship/Gun");
```

Parentage and world matrices are Compages systems (`SpatialGraph`,
`TransformStore`), not delegated to EnTT. `world.update()` propagates them.

## Behaviors

```cpp
struct Spin : scene::Behavior
{
    explicit Spin(float p_speed) : speed(p_speed) {}

    void update(float p_dt) override
    {
        transform().rotateY(speed * p_dt);
    }

    float speed;
};

ship.add<Spin>(2.0f);
world.update(frame);   // start() once, then update(frame.elapsed)
```

Inside a behavior, `entity()`, `transform()`, `world()`, `input()` and
`frame()` reach what it works on. `get<Spin>()`, `has<Spin>()` and
`remove<Spin>()` work on the entity as for components.

## The Scene: a view on the World

```cpp
scene::Scene scene(world);   // owns its AssetManager, or shares one

scene.camera().position(0, 2, 6).add<scene::Orbit>();
scene.sun();
scene.box("Crate", scene::texture("wooden-crate.jpg")).add<Spin>(1.0f);
scene.sphere("Ball", scene::color(1.0f, 0.3f, 0.2f)).position(2, 0, 0);
COMPAGES_TRY(scene.prepare());

// every frame
scene.draw(frame);           // update, then render with the active camera
```

For several views of the same World, update once and render each camera:

```cpp
scene.update(frame);
scene.render(eye);
map.render(top);             // a second Scene over the same World
```

## Assets and instances

Loading and instantiation are separate:

```cpp
scene::PrefabId city;
COMPAGES_TRY_ASSIGN(city, scene.assets().load("city.glb"));   // no World
scene::Entity first;
COMPAGES_TRY_ASSIGN(first, scene.instantiate(city));
```

For the common case, `scene.load("city.glb")` is exactly load followed by
instantiate. A prefab can be instantiated several times; skin joints and
animation targets are remapped per instance.

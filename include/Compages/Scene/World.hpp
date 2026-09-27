//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Scene/EntityId.hpp"
#include "Compages/Scene/Frame.hpp"
#include "Compages/Scene/Joint.hpp"
#include "Compages/Scene/KinematicSystem.hpp"
#include "Compages/Scene/LookAt.hpp"
#include "Compages/Scene/SpatialGraph.hpp"
#include "Compages/Scene/TransformStore.hpp"
#include "Compages/Scene/TransformSystem.hpp"

#include <entt/entity/registry.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ****************************************************************************
//! \file
//! \brief The World: entities, their components, their hierarchy and their
//! behaviors. Headless: nothing here needs a GPU.
//!
//! The syntax is the one of flecs. An entity is created by name and given
//! components, which are plain structs:
//! \code
//! struct Position { float x, y; };
//! struct Velocity { float x, y; };
//!
//! scene::World world;
//! world.entity("player").set(Position{ 0, 0 }).set(Velocity{ 1, 0 });
//!
//! world.each<Position, Velocity>([](scene::Entity, Position& p, Velocity
//! const& v) {
//!     p.x += v.x;
//!     p.y += v.y;
//! });
//! \endcode
//!
//! Every entity also has a place in space, which can be chained the way
//! three.js does, and a parent:
//! \code
//! scene::Entity robot = world.entity("Robot").position(0, 1, 0);
//! robot.child("Head").position(0, 0.8f, 0).scale(0.5f);
//! \endcode
//!
//! A robot arm is the same hierarchy with joints between its links:
//! \code
//! scene::Entity base = world.entity("Base");
//! scene::Entity arm = base.child("Arm").position(0, 0, 0.5f)
//!                         .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
//! arm.angle(30.0_deg);
//! \endcode
// ****************************************************************************

namespace scene
{

class World;
class Entity;

// ****************************************************************************
//! \brief A piece of code attached to an entity and run every frame, the way
//! a Unity MonoBehaviour is.
//!
//! Derive from it, override update(), and add it to an entity like a
//! component:
//! \code
//! struct Spin: scene::Behavior
//! {
//!     float speed = 1.0f;
//!     explicit Spin(float p_speed) : speed(p_speed) {}
//!
//!     void update(float p_dt) override
//!     {
//!         transform().rotateY(speed * p_dt);
//!     }
//! };
//!
//! cube.add<Spin>(2.0f);    // turns two radians a second
//! \endcode
//!
//! World::update() runs them, in the order they were added, after calling
//! start() once on the ones that are new. An entity can have several.
// ****************************************************************************
class Behavior
{
public:

    virtual ~Behavior() = default;

    //! \brief Called once, at the first update after the behavior was added.
    virtual void start()
    {
        /* do nothing */
    }

    //! \brief Called at every update of the World.
    //! \param[in] p_dt the seconds since the previous frame.
    virtual void update(float p_dt)
    {
        (void)p_dt;
    }

protected:

    //! \brief The entity this behavior is attached to.
    [[nodiscard]] Entity entity() const;

    //! \brief The place of that entity, to be changed.
    [[nodiscard]] LocalTransformView transform() const;

    //! \brief The world the entity lives in.
    [[nodiscard]] World& world() const
    {
        return *m_world;
    }

    //! \brief The mouse and the keys during this frame.
    [[nodiscard]] Input const& input() const;

    //! \brief The whole frame: size, time and input.
    [[nodiscard]] Frame const& frame() const;

private:

    friend class World;

    World* m_world = nullptr;
    EntityId m_entity{};
    bool m_started = false;
};

// ****************************************************************************
//! \brief The source of truth of a simulation: the entities and everything
//! attached to them.
//!
//! A World never touches the GPU. It never opens a window and never draws: a
//! test, a tool or a server runs the same simulation without a device. What
//! is drawn, and how, is the business of a scene::Scene looking at the World.
// ****************************************************************************
class World
{
public:

    World() = default;
    World(World const&) = delete;
    World& operator=(World const&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Create an entity, named or not.
    //!
    //! \code
    //! scene::Entity player = world.entity("player");
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity entity(std::string p_name = {});

    //! \brief The handle of an entity known by its id.
    [[nodiscard]] Entity entity(EntityId p_id);

    // ------------------------------------------------------------------------
    //! \brief The entity at the end of a path of names, starting from the
    //! entities without a parent, or an empty handle.
    //!
    //! \code
    //! scene::Entity leg = world.lookup("Robot/Body/LeftLeg");
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity lookup(std::string_view p_path);

    // ------------------------------------------------------------------------
    //! \brief Call a function for every entity having all the components T,
    //! with the entity and a reference to each component.
    //!
    //! \code
    //! world.each<Position, Velocity>([](scene::Entity e, Position& p,
    //! Velocity& v) { ... });
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename... T, typename F>
    void each(F&& p_callback);

    //! \brief Same, read only. The entity is given by its id.
    template <typename... T, typename F>
    void each(F&& p_callback) const
    {
        m_registry.view<T...>().each(
            [&p_callback](entt::entity p_entity, T const&... p_components)
            { std::invoke(p_callback, EntityId(p_entity), p_components...); });
    }

    // ------------------------------------------------------------------------
    //! \brief Advance the World by one frame: run the behaviors, then compute
    //! the place in the world of every entity that moved.
    // ------------------------------------------------------------------------
    void update(Frame const& p_frame);

    // ------------------------------------------------------------------------
    //! \brief Only compute the places in the world of the entities that moved,
    //! parents before children. Joints are turned into local places first.
    //! Run by update(Frame).
    // ------------------------------------------------------------------------
    void update();

    //! \brief The frame given to the last update(Frame), for the behaviors.
    [[nodiscard]] Frame const& frame() const
    {
        return m_frame;
    }

    //! \brief How many entities are alive.
    [[nodiscard]] std::size_t living() const
    {
        return m_living;
    }

    // ------------------------------------------------------------------------
    // What follows works on ids and is what the Entity handle is made of. It
    // is also what the renderer, the loaders and the serializer use.
    // ------------------------------------------------------------------------

    //! \brief Create an entity and return its id. The entity has a place in
    //! space and no parent.
    [[nodiscard]] EntityId create(std::string p_name = {});

    //! \brief Destroy an entity, its children first, and all their
    //! components.
    void destroy(EntityId p_entity);

    [[nodiscard]] bool alive(EntityId p_entity) const;

    //! \brief The name of an entity, or an empty string.
    [[nodiscard]] std::string const& name(EntityId p_entity) const;
    void setName(EntityId p_entity, std::string p_name);

    //! \brief A disabled entity is skipped by the renderer and the behaviors,
    //! and so are its children.
    void setEnabled(EntityId p_entity, bool p_enabled);
    [[nodiscard]] bool enabled(EntityId p_entity) const;
    [[nodiscard]] bool enabledInHierarchy(EntityId p_entity) const;

    [[nodiscard]] SpatialGraph& spatial()
    {
        return m_graph;
    }
    [[nodiscard]] SpatialGraph const& spatial() const
    {
        return m_graph;
    }

    //! \brief Change or clear the parent of an entity. Refused when either is
    //! dead or when it would make a cycle.
    [[nodiscard]] compages::Status
    setParent(EntityId p_child,
              EntityId p_parent,
              ReparentPolicy p_policy = ReparentPolicy::KeepLocal);

    [[nodiscard]] EntityId parent(EntityId p_entity) const;
    [[nodiscard]] EntityId firstChild(EntityId p_entity) const;
    [[nodiscard]] EntityId nextSibling(EntityId p_entity) const;

    //! \brief The entity at the end of a path of names under p_root, as
    //! "Body/LeftLeg", or an empty id.
    [[nodiscard]] EntityId find(EntityId p_root, std::string_view p_path) const;

    //! \brief How many entities hang under this one, itself included.
    [[nodiscard]] std::size_t descendantCount(EntityId p_entity) const;

    [[nodiscard]] TransformStore& transforms()
    {
        return m_transforms;
    }
    [[nodiscard]] TransformStore const& transforms() const
    {
        return m_transforms;
    }

    //! \brief The local place of an entity, to be changed. Its place in the
    //! world, and that of its children, are computed again at the next
    //! update().
    [[nodiscard]] LocalTransformView transform(EntityId p_entity);

    //! \brief A copy of the local place of an entity.
    [[nodiscard]] LocalTransform transform(EntityId p_entity) const;

    //! \brief The place of an entity in the world, as of the last update().
    [[nodiscard]] Matrix44f const& worldMatrix(EntityId p_entity) const;

    //! \brief The EnTT view of the entities having all the components T.
    template <typename... T>
    [[nodiscard]] auto view()
    {
        return m_registry.view<T...>();
    }

    template <typename... T>
    [[nodiscard]] auto view() const
    {
        return m_registry.view<T...>();
    }

    //! \brief Attach a component, replacing the one of that type.
    template <typename T>
    T& add(EntityId p_entity, T p_component = T{})
    {
        assert(alive(p_entity) && "World::add on a dead entity");
        return m_registry.emplace_or_replace<T>(p_entity.native(),
                                                std::move(p_component));
    }

    //! \brief Detach a component, if there is one.
    template <typename T>
    void remove(EntityId p_entity)
    {
        if (alive(p_entity))
        {
            m_registry.remove<T>(p_entity.native());
        }
    }

    template <typename T>
    [[nodiscard]] bool has(EntityId p_entity) const
    {
        return alive(p_entity) && m_registry.all_of<T>(p_entity.native());
    }

    //! \brief The component, or nullptr when the entity has none.
    template <typename T>
    [[nodiscard]] T* tryGet(EntityId p_entity)
    {
        return alive(p_entity) ? m_registry.try_get<T>(p_entity.native())
                               : nullptr;
    }

    template <typename T>
    [[nodiscard]] T const* tryGet(EntityId p_entity) const
    {
        return alive(p_entity) ? m_registry.try_get<T>(p_entity.native())
                               : nullptr;
    }

    //! \brief The component, which the entity must have.
    template <typename T>
    [[nodiscard]] T& get(EntityId p_entity)
    {
        assert(has<T>(p_entity) && "World::get on an entity without one");
        return m_registry.get<T>(p_entity.native());
    }

    template <typename T>
    [[nodiscard]] T const& get(EntityId p_entity) const
    {
        assert(has<T>(p_entity) && "World::get on an entity without one");
        return m_registry.get<T>(p_entity.native());
    }

    //! \brief Attach a behavior to an entity. What Entity::add<T>() does when
    //! T derives from Behavior.
    Behavior& addBehavior(EntityId p_entity,
                          std::unique_ptr<Behavior> p_behavior);

    //! \brief How many behaviors an entity has.
    [[nodiscard]] std::size_t behaviorCount(EntityId p_entity) const;

    //! \brief The first behavior of type T of an entity, or nullptr.
    template <typename T>
    [[nodiscard]] T* behavior(EntityId p_entity)
    {
        Behaviors* behaviors = tryGet<Behaviors>(p_entity);
        if (behaviors == nullptr)
        {
            return nullptr;
        }
        for (std::unique_ptr<Behavior> const& behavior : behaviors->list)
        {
            if (T* found = dynamic_cast<T*>(behavior.get()))
            {
                return found;
            }
        }
        return nullptr;
    }

    //! \brief Take the behaviors of type T off an entity. Not from inside one
    //! of them: it would be destroyed while it runs.
    template <typename T>
    void removeBehaviors(EntityId p_entity)
    {
        if (Behaviors* behaviors = tryGet<Behaviors>(p_entity))
        {
            std::erase_if(
                behaviors->list,
                [](std::unique_ptr<Behavior> const& p_behavior)
                { return dynamic_cast<T*>(p_behavior.get()) != nullptr; });
        }
    }

private:

    void destroyRecursive(EntityId p_entity);
    void runBehaviors();

    struct Name
    {
        std::string value;
    };

    struct Disabled
    {
    };

    //! \brief The behaviors of one entity, as a component.
    struct Behaviors
    {
        Behaviors() = default;
        Behaviors(Behaviors const&) = delete;
        Behaviors& operator=(Behaviors const&) = delete;
        Behaviors(Behaviors&&) = default;
        Behaviors& operator=(Behaviors&&) = default;

        std::vector<std::unique_ptr<Behavior>> list;
    };

    entt::registry m_registry;
    std::size_t m_living = 0u;
    SpatialGraph m_graph;
    TransformStore m_transforms;
    KinematicSystem m_kinematic_system;
    TransformSystem m_transform_system;
    Frame m_frame{};

    static const std::string s_empty_name;
};

// ****************************************************************************
//! \brief An entity of a World, and everything that can be done with it.
//!
//! A handle: the World and the id. Copying it is free, and it goes on naming
//! the same entity until that entity is destroyed; then it tests false.
//! Everything that changes the entity returns the handle, so calls chain:
//! \code
//! scene::Entity lamp = world.entity("Lamp")
//!                           .position(0, 3, 0)
//!                           .set(scene::PointLight{ .color = { 1, 0.8f, 0.6f }
//!                           });
//! \endcode
// ****************************************************************************
class Entity
{
public:

    //! \brief An empty handle, naming nothing.
    Entity() = default;

    Entity(World& p_world, EntityId p_id) : m_world(&p_world), m_id(p_id) {}

    //! \brief The id of the entity inside its World.
    [[nodiscard]] EntityId id() const
    {
        return m_id;
    }

    //! \brief The id, for the functions that take one.
    [[nodiscard]] operator EntityId() const
    {
        return m_id;
    }

    //! \brief Does the handle name an entity still alive?
    [[nodiscard]] explicit operator bool() const
    {
        return (m_world != nullptr) && m_world->alive(m_id);
    }

    [[nodiscard]] bool operator==(Entity const& p_other) const
    {
        return (m_world == p_other.m_world) && (m_id == p_other.m_id);
    }

    //! \brief The World the entity lives in.
    [[nodiscard]] World& world() const
    {
        assert(m_world != nullptr && "an empty entity handle has no world");
        return *m_world;
    }

    // ------------------------------------------------------------------------
    // Components
    // ------------------------------------------------------------------------

    //! \brief Attach a component, replacing the one of that type.
    //! \code
    //! player.set(Velocity{ 1, 0 });
    //! \endcode
    template <typename T>
    Entity& set(T p_component)
    {
        world().add(m_id, std::move(p_component));
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Attach a component built from the arguments, or a behavior when
    //! T derives from Behavior.
    //!
    //! \code
    //! enemy.add<Hostile>();           // a tag: a component without data
    //! cube.add<Spin>(2.0f);           // a behavior, built with Spin(2.0f)
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename T, typename... Args>
    Entity& add(Args&&... p_args)
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            world().addBehavior(
                m_id, std::make_unique<T>(std::forward<Args>(p_args)...));
        }
        else if constexpr (std::is_constructible_v<T, Args...>)
        {
            world().add(m_id, T(std::forward<Args>(p_args)...));
        }
        else
        {
            world().add(m_id, T{ std::forward<Args>(p_args)... });
        }
        return *this;
    }

    //! \brief Detach the component of type T, or the behaviors of type T.
    template <typename T>
    Entity& remove()
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            world().template removeBehaviors<T>(m_id);
        }
        else
        {
            world().template remove<T>(m_id);
        }
        return *this;
    }

    //! \brief Has the entity a component, or a behavior, of type T?
    template <typename T>
    [[nodiscard]] bool has() const
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            return find<T>() != nullptr;
        }
        else
        {
            return (m_world != nullptr) && m_world->template has<T>(m_id);
        }
    }

    //! \brief The component, which the entity must have.
    template <typename T>
    [[nodiscard]] T& get() const
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            T* behavior = find<T>();
            assert((behavior != nullptr) &&
                   "Entity::get of a missing behavior");
            return *behavior;
        }
        else
        {
            return world().template get<T>(m_id);
        }
    }

    //! \brief The component, or the behavior, of type T; nullptr when the
    //! entity has none.
    template <typename T>
    [[nodiscard]] T* find() const
    {
        if (m_world == nullptr)
        {
            return nullptr;
        }
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            return m_world->template behavior<T>(m_id);
        }
        else
        {
            return m_world->template tryGet<T>(m_id);
        }
    }

    // ------------------------------------------------------------------------
    // Identity
    // ------------------------------------------------------------------------

    [[nodiscard]] std::string const& name() const
    {
        return world().name(m_id);
    }

    Entity& name(std::string p_name)
    {
        world().setName(m_id, std::move(p_name));
        return *this;
    }

    //! \brief Show and run the entity and its children, or neither.
    Entity& enable(bool p_enabled = true)
    {
        world().setEnabled(m_id, p_enabled);
        return *this;
    }

    [[nodiscard]] bool enabled() const
    {
        return world().enabledInHierarchy(m_id);
    }

    //! \brief Destroy the entity and its children.
    void destroy() const
    {
        world().destroy(m_id);
    }

    // ------------------------------------------------------------------------
    // Hierarchy
    // ------------------------------------------------------------------------

    //! \brief Create an entity hanging from this one. It moves with it.
    [[nodiscard]] Entity child(std::string p_name = {}) const;

    //! \brief Hang this entity from another one, keeping its local place.
    Entity& parent(Entity const& p_parent)
    {
        [[maybe_unused]] const auto done =
            bool(world().setParent(m_id, p_parent.m_id));
        assert(done && "parent(): a dead entity, or a cycle");
        return *this;
    }

    //! \brief The entity this one hangs from, or an empty handle.
    [[nodiscard]] Entity parent() const
    {
        return Entity(world(), world().parent(m_id));
    }

    //! \brief The entity at the end of a path of names under this one, as
    //! "Body/LeftLeg", or an empty handle.
    [[nodiscard]] Entity lookup(std::string_view p_path) const
    {
        return Entity(world(), world().find(m_id, p_path));
    }

    //! \brief Call a function with each child, in order.
    template <typename F>
    void children(F&& p_callback) const
    {
        EntityId child = world().firstChild(m_id);
        while (child.valid())
        {
            const EntityId next = world().nextSibling(child);
            std::invoke(p_callback, Entity(world(), child));
            child = next;
        }
    }

    // ------------------------------------------------------------------------
    // Place, relative to the parent. On a joint, the place of the joint frame
    // when the joint is at zero: the joint then moves the entity from there.
    // ------------------------------------------------------------------------

    Entity& position(float p_x, float p_y, float p_z)
    {
        return position(Vector3f(p_x, p_y, p_z));
    }

    Entity& position(Vector3f const& p_position)
    {
        place().position = p_position;
        return *this;
    }

    [[nodiscard]] Vector3f position() const
    {
        if (LocalTransform const* origin = jointOrigin())
        {
            return origin->position;
        }
        return world().transforms().position(m_id);
    }

    Entity& rotation(Quatf const& p_rotation)
    {
        place().rotation = p_rotation;
        return *this;
    }

    //! \brief Set the orientation to an angle, in radians, around an axis.
    Entity& rotation(float p_radians, Vector3f const& p_axis)
    {
        return rotation(Quatf::fromAngleAxis(units::angle::radian_t(p_radians),
                                             vector::normalize(p_axis)));
    }

    [[nodiscard]] Quatf rotation() const
    {
        if (LocalTransform const* origin = jointOrigin())
        {
            return origin->rotation;
        }
        return world().transforms().rotation(m_id);
    }

    //! \brief Turn by an angle, in radians, around an axis of the entity.
    Entity& rotate(float p_radians, Vector3f const& p_axis)
    {
        place().rotate(p_radians, p_axis);
        return *this;
    }

    Entity& scale(float p_scale)
    {
        return scale(Vector3f(p_scale, p_scale, p_scale));
    }

    Entity& scale(float p_x, float p_y, float p_z)
    {
        return scale(Vector3f(p_x, p_y, p_z));
    }

    Entity& scale(Vector3f const& p_scale)
    {
        place().scale = p_scale;
        return *this;
    }

    [[nodiscard]] Vector3f scale() const
    {
        if (LocalTransform const* origin = jointOrigin())
        {
            return origin->scale;
        }
        return world().transforms().scale(m_id);
    }

    //! \brief Turn so as to face a point, given in the parent's axes: the
    //! entity's -z axis points at it, as a camera looks.
    Entity& lookAt(Vector3f const& p_target,
                   Vector3f const& p_up = Vector3f(0.0f, 1.0f, 0.0f))
    {
        scene::lookAt(place(), p_target, p_up);
        return *this;
    }

    Entity& lookAt(float p_x, float p_y, float p_z)
    {
        return lookAt(Vector3f(p_x, p_y, p_z));
    }

    // ------------------------------------------------------------------------
    // Joints: what moves a link of a robot relative to its parent link
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief Make the entity turn around an axis, within an interval of
    //! angles. Its current place becomes the place of the joint at zero.
    //!
    //! \code
    //! arm.revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg).angle(30.0_deg);
    //! wheel.revolute({ 1, 0, 0 });    // no bounds: a continuous joint
    //! \endcode
    // ------------------------------------------------------------------------
    Entity& revolute(Vector3f const& p_axis,
                     units::angle::radian_t p_min = units::angle::radian_t(
                         -std::numeric_limits<double>::infinity()),
                     units::angle::radian_t p_max = units::angle::radian_t(
                         std::numeric_limits<double>::infinity()))
    {
        RevoluteJoint joint{ .origin = takeOrigin(),
                             .axis = vector::normalize(p_axis) };
        joint.state.position.min = p_min;
        joint.state.position.max = p_max;
        world().add(m_id, std::move(joint));
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Make the entity slide along an axis, within an interval of
    //! offsets. Its current place becomes the place of the joint at zero.
    //!
    //! \code
    //! slider.prismatic({ 0, 0, 1 }, 0.0_m, 0.3_m).offset(0.1_m);
    //! \endcode
    // ------------------------------------------------------------------------
    Entity& prismatic(Vector3f const& p_axis,
                      units::length::meter_t p_min = units::length::meter_t(
                          -std::numeric_limits<double>::infinity()),
                      units::length::meter_t p_max = units::length::meter_t(
                          std::numeric_limits<double>::infinity()))
    {
        PrismaticJoint joint{ .origin = takeOrigin(),
                              .axis = vector::normalize(p_axis) };
        joint.state.position.min = p_min;
        joint.state.position.max = p_max;
        world().add(m_id, std::move(joint));
        return *this;
    }

    //! \brief Set the angle of the revolute joint. Kept inside its bounds when
    //! the World is updated.
    Entity& angle(units::angle::radian_t p_angle)
    {
        world().template get<RevoluteJoint>(m_id).state.position.value =
            p_angle;
        return *this;
    }

    //! \brief The angle of the revolute joint, as last set.
    [[nodiscard]] units::angle::radian_t angle() const
    {
        return world().template get<RevoluteJoint>(m_id).state.position.value;
    }

    //! \brief Set the offset of the prismatic joint. Kept inside its bounds
    //! when the World is updated.
    Entity& offset(units::length::meter_t p_offset)
    {
        world().template get<PrismaticJoint>(m_id).state.position.value =
            p_offset;
        return *this;
    }

    //! \brief The offset of the prismatic joint, as last set.
    [[nodiscard]] units::length::meter_t offset() const
    {
        return world().template get<PrismaticJoint>(m_id).state.position.value;
    }

    //! \brief The local place, to be changed field by field. On a joint, it is
    //! overwritten by the joint at the next update: change position() or
    //! rotation() instead.
    [[nodiscard]] LocalTransformView transform() const
    {
        return world().transform(m_id);
    }

    //! \brief The place in the world, as of the last update of the World.
    [[nodiscard]] Matrix44f const& worldMatrix() const
    {
        return world().worldMatrix(m_id);
    }

    //! \brief Where the entity is in the world, as of the last update.
    [[nodiscard]] Vector3f worldPosition() const
    {
        Matrix44f const& m = worldMatrix();
        return Vector3f(m[3].x, m[3].y, m[3].z);
    }

private:

    //! \brief The origin of the joint of the entity, or nullptr without one.
    [[nodiscard]] LocalTransform* jointOrigin() const
    {
        if (m_world == nullptr)
        {
            return nullptr;
        }
        if (auto* joint = m_world->template tryGet<RevoluteJoint>(m_id))
        {
            return &joint->origin;
        }
        if (auto* joint = m_world->template tryGet<PrismaticJoint>(m_id))
        {
            return &joint->origin;
        }
        return nullptr;
    }

    //! \brief What position(), rotation() and scale() change: the joint
    //! origin, or else the local place.
    [[nodiscard]] LocalTransformView place() const
    {
        if (LocalTransform* origin = jointOrigin())
        {
            return LocalTransformView(
                origin->position, origin->rotation, origin->scale);
        }
        return transform();
    }

    //! \brief The origin of a new joint: that of the joint being replaced, or
    //! the local place. Removes the joint being replaced.
    [[nodiscard]] LocalTransform takeOrigin() const
    {
        LocalTransform origin = world().transforms().local(m_id);
        if (LocalTransform const* previous = jointOrigin())
        {
            origin = *previous;
        }
        world().template remove<RevoluteJoint>(m_id);
        world().template remove<PrismaticJoint>(m_id);
        return origin;
    }

    World* m_world = nullptr;
    EntityId m_id{};
};

// ----------------------------------------------------------------------------
inline Entity World::entity(std::string p_name)
{
    return Entity(*this, create(std::move(p_name)));
}

inline Entity World::entity(EntityId p_id)
{
    return Entity(*this, p_id);
}

template <typename... T, typename F>
void World::each(F&& p_callback)
{
    m_registry.view<T...>().each(
        [this, &p_callback](entt::entity p_entity, T&... p_components)
        {
            std::invoke(
                p_callback, Entity(*this, EntityId(p_entity)), p_components...);
        });
}

inline Entity Entity::child(std::string p_name) const
{
    Entity kid = world().entity(std::move(p_name));
    kid.parent(*this);
    return kid;
}

inline Entity Behavior::entity() const
{
    return Entity(*m_world, m_entity);
}

inline LocalTransformView Behavior::transform() const
{
    return m_world->transform(m_entity);
}

inline Input const& Behavior::input() const
{
    return m_world->frame().input;
}

inline Frame const& Behavior::frame() const
{
    return m_world->frame();
}

} // namespace scene

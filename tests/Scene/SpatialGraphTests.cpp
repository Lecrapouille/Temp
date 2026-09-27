//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/Scene/SpatialGraph.hpp"
#include "Compages/Scene/TransformStore.hpp"
#include "Compages/Scene/World.hpp"

//------------------------------------------------------------------------------
TEST(SpatialGraph, AttachIsIdempotent)
{
    scene::World world;
    scene::SpatialGraph graph;

    scene::EntityId e = world.create();
    scene::NodeId a = graph.attach(e);
    scene::NodeId b = graph.attach(e);
    ASSERT_EQ(a, b);
    ASSERT_EQ(graph.size(), 1u);
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, ParentAndChildLink)
{
    scene::World world;
    scene::SpatialGraph graph;

    scene::EntityId ea = world.create();
    scene::EntityId eb = world.create();
    scene::NodeId a = graph.attach(ea);
    scene::NodeId b = graph.attach(eb);

    ASSERT_TRUE(bool(graph.setParent(b, a)));
    ASSERT_EQ(graph.parent(b), a);
    ASSERT_EQ(graph.firstChild(a), b);
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, RefusesACycle)
{
    scene::World world;
    scene::SpatialGraph graph;

    scene::EntityId ea = world.create();
    scene::EntityId eb = world.create();
    scene::NodeId a = graph.attach(ea);
    scene::NodeId b = graph.attach(eb);
    ASSERT_TRUE(bool(graph.setParent(b, a)));

    auto cycled = graph.setParent(a, b);
    ASSERT_FALSE(bool(cycled));
    ASSERT_THAT(cycled.error(), HasSubstr("cycle"));
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, DestroySubtree)
{
    scene::World world;
    scene::SpatialGraph graph;

    scene::EntityId ea = world.create();
    scene::EntityId eb = world.create();
    scene::EntityId ec = world.create();
    scene::NodeId a = graph.attach(ea);
    scene::NodeId b = graph.attach(eb);
    scene::NodeId c = graph.attach(ec);
    ASSERT_TRUE(bool(graph.setParent(b, a)));
    ASSERT_TRUE(bool(graph.setParent(c, b)));
    ASSERT_EQ(graph.descendantCount(a), 3u);

    graph.destroy(a);
    ASSERT_FALSE(graph.alive(a));
    ASSERT_FALSE(graph.alive(b));
    ASSERT_FALSE(graph.alive(c));
    ASSERT_EQ(graph.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, KeepWorldReparentPreservesWorldPose)
{
    scene::World world;
    scene::SpatialGraph graph;
    scene::TransformStore transforms;

    scene::EntityId er = world.create();
    scene::EntityId ea = world.create();
    scene::EntityId eb = world.create();
    scene::EntityId ec = world.create();
    transforms.allocate(er);
    transforms.allocate(ea);
    transforms.allocate(eb);
    transforms.allocate(ec);

    scene::NodeId a = graph.attach(ea);
    scene::NodeId b = graph.attach(eb);
    scene::NodeId c = graph.attach(ec);
    ASSERT_TRUE(bool(graph.setParent(b, a)));
    ASSERT_TRUE(bool(graph.setParent(c, b)));

    // A at (10, 0, 0). B at (0, 5, 0) relative to A. C at (0, 2, 0) relative
    // to B. World position of C is (10, 7, 0).
    transforms.localMutable(ea).position = Vector3f(10.0f, 0.0f, 0.0f);
    transforms.localMutable(eb).position = Vector3f(0.0f, 5.0f, 0.0f);
    transforms.localMutable(ec).position = Vector3f(0.0f, 2.0f, 0.0f);

    // Manually build world matrices (no TransformSystem here).
    transforms.setWorld(ea, transforms.local(ea).matrix());
    transforms.setWorld(eb,
                        transforms.local(eb).matrix() * transforms.world(ea));
    transforms.setWorld(ec,
                        transforms.local(ec).matrix() * transforms.world(eb));

    // Reparent C from B to A, keeping the world pose.
    ASSERT_TRUE(bool(graph.setParent(c, a, scene::ReparentPolicy::KeepWorld,
                                     &transforms)));

    // C's world position should still be (10, 7, 0), so its new local
    // relative to A should be (0, 7, 0).
    ASSERT_NEAR(transforms.local(ec).position.x, 0.0f, 1.0e-3f);
    ASSERT_NEAR(transforms.local(ec).position.y, 7.0f, 1.0e-3f);
}

//==============================================================================
// OpenGLCppWrapper: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
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
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "World/EntityRegistry.hpp"

//------------------------------------------------------------------------------
TEST(EntityRegistry, EmptyHandleIsNeverAlive)
{
    world::EntityRegistry registry;
    world::Entity none;
    ASSERT_FALSE(none.valid());
    ASSERT_FALSE(registry.alive(none));
    ASSERT_EQ(registry.living(), 0u);
}

//------------------------------------------------------------------------------
TEST(EntityRegistry, CreateReturnsAliveHandle)
{
    world::EntityRegistry registry;
    world::Entity a = registry.create();
    ASSERT_TRUE(a.valid());
    ASSERT_TRUE(registry.alive(a));
    ASSERT_EQ(registry.living(), 1u);
}

//------------------------------------------------------------------------------
TEST(EntityRegistry, DestroyMakesHandleStale)
{
    world::EntityRegistry registry;
    world::Entity a = registry.create();
    registry.destroy(a);
    ASSERT_FALSE(registry.alive(a));
    ASSERT_EQ(registry.living(), 0u);
}

//------------------------------------------------------------------------------
TEST(EntityRegistry, ReusedSlotHasNewGeneration)
{
    world::EntityRegistry registry;
    world::Entity first = registry.create();
    registry.destroy(first);
    world::Entity second = registry.create();
    ASSERT_TRUE(registry.alive(second));
    ASSERT_FALSE(registry.alive(first));
    ASSERT_EQ(second.index(), first.index());
    ASSERT_NE(second.generation(), first.generation());
}

//------------------------------------------------------------------------------
TEST(EntityRegistry, DestroyIsIdempotent)
{
    world::EntityRegistry registry;
    world::Entity a = registry.create();
    registry.destroy(a);
    registry.destroy(a); // twice; no crash, still not alive.
    ASSERT_FALSE(registry.alive(a));
    ASSERT_EQ(registry.living(), 0u);
}

//------------------------------------------------------------------------------
TEST(EntityRegistry, ForEachVisitsLivingOnly)
{
    world::EntityRegistry registry;
    world::Entity a = registry.create();
    world::Entity b = registry.create();
    world::Entity c = registry.create();
    registry.destroy(b);

    std::size_t seen = 0u;
    registry.forEach([&](world::Entity) { ++seen; });
    ASSERT_EQ(seen, 2u);
    ASSERT_TRUE(registry.alive(a));
    ASSERT_FALSE(registry.alive(b));
    ASSERT_TRUE(registry.alive(c));
}

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

#include "World/ComponentStore.hpp"

namespace
{
struct Payload
{
    int x = 0;
};
} // namespace

//------------------------------------------------------------------------------
TEST(ComponentStore, StartsEmpty)
{
    world::ComponentStore<Payload> store;
    ASSERT_EQ(store.size(), 0u);
    ASSERT_TRUE(store.empty());
}

//------------------------------------------------------------------------------
TEST(ComponentStore, AddAndGet)
{
    world::ComponentStore<Payload> store;
    world::Entity a(0u, 1u);
    world::Entity b(1u, 1u);
    store.add(a, Payload{ 42 });
    store.add(b, Payload{ 7 });

    ASSERT_TRUE(store.has(a));
    ASSERT_TRUE(store.has(b));
    ASSERT_EQ(store.get(a).x, 42);
    ASSERT_EQ(store.get(b).x, 7);
    ASSERT_EQ(store.size(), 2u);
}

//------------------------------------------------------------------------------
TEST(ComponentStore, ReplaceExisting)
{
    world::ComponentStore<Payload> store;
    world::Entity a(0u, 1u);
    store.add(a, Payload{ 1 });
    store.add(a, Payload{ 2 });
    ASSERT_EQ(store.size(), 1u);
    ASSERT_EQ(store.get(a).x, 2);
}

//------------------------------------------------------------------------------
TEST(ComponentStore, RemoveSwapsWithLast)
{
    world::ComponentStore<Payload> store;
    world::Entity a(0u, 1u);
    world::Entity b(1u, 1u);
    world::Entity c(2u, 1u);
    store.add(a, Payload{ 1 });
    store.add(b, Payload{ 2 });
    store.add(c, Payload{ 3 });
    store.remove(b);
    ASSERT_FALSE(store.has(b));
    ASSERT_TRUE(store.has(a));
    ASSERT_TRUE(store.has(c));
    ASSERT_EQ(store.size(), 2u);
    ASSERT_EQ(store.get(a).x, 1);
    ASSERT_EQ(store.get(c).x, 3);
}

//------------------------------------------------------------------------------
TEST(ComponentStore, StaleHandleFromReusedSlot)
{
    world::ComponentStore<Payload> store;
    world::Entity old_a(0u, 1u);
    store.add(old_a, Payload{ 42 });

    // Same slot, next generation: the leftover component of old_a must not
    // be visible under new_a's handle.
    world::Entity new_a(0u, 2u);
    ASSERT_FALSE(store.has(new_a));
    ASSERT_EQ(store.tryGet(new_a), nullptr);
}

//------------------------------------------------------------------------------
TEST(ComponentStore, EntitiesAndComponentsSpans)
{
    world::ComponentStore<Payload> store;
    world::Entity a(0u, 1u);
    world::Entity b(1u, 1u);
    store.add(a, Payload{ 1 });
    store.add(b, Payload{ 2 });

    ASSERT_EQ(store.entities().size(), 2u);
    ASSERT_EQ(store.components().size(), 2u);
    int total = 0;
    for (auto const& v : store.components())
    {
        total += v.x;
    }
    ASSERT_EQ(total, 3);
}

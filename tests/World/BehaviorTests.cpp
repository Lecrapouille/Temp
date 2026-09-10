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

#include "World/Behavior.hpp"
#include "World/BehaviorSystem.hpp"
#include "World/InputState.hpp"
#include "World/World.hpp"

namespace
{

int g_calls = 0;

void countBehavior(world::World& /*p_world*/,
                   world::Entity /*p_entity*/,
                   float /*p_dt*/,
                   world::InputState const& /*p_input*/,
                   void* /*p_user*/)
{
    ++g_calls;
}

} // namespace

//------------------------------------------------------------------------------
TEST(BehaviorSystem, RunsAttachedCallbacks)
{
    world::World world;
    world::Entity entity = world.create("Actor");
    world::Behavior behavior;
    behavior.callback = countBehavior;
    world.add(entity, behavior);

    world::InputState input;
    g_calls = 0;
    world::updateBehaviors(world, 0.016f, input);
    ASSERT_EQ(g_calls, 1);
}

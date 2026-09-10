//=============================================================================
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
// OpenGLCppWrapper is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "World/BehaviorSystem.hpp"

#include "World/Behavior.hpp"
#include "World/World.hpp"

namespace world
{

//------------------------------------------------------------------------------
void updateBehaviors(World& p_world, float p_dt, InputState const& p_input)
{
    ComponentStore<Behavior>& store = p_world.components<Behavior>();
    std::span<Entity const> entities = store.entities();
    std::span<Behavior> behaviors = store.components();
    for (std::size_t i = 0u; i < entities.size(); ++i)
    {
        if (behaviors[i].callback != nullptr)
        {
            behaviors[i].callback(
                p_world, entities[i], p_dt, p_input, behaviors[i].user);
        }
    }
}

} // namespace world

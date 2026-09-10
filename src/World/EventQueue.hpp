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

#pragma once

#include "World/Entity.hpp"

#include <span>
#include <vector>

namespace world
{

// ****************************************************************************
//! \brief What happened this frame.
// ****************************************************************************
enum class EventKind
{
    Collision,
};

// ****************************************************************************
//! \brief A lightweight event emitted by physics or gameplay systems.
// ****************************************************************************
struct Event
{
    EventKind kind = EventKind::Collision;
    Entity a{};
    Entity b{};
};

// ****************************************************************************
//! \brief FIFO queue cleared once per frame after dispatch.
// ****************************************************************************
class EventQueue
{
public:

    void push(Event p_event) { m_events.push_back(p_event); }

    [[nodiscard]] std::span<Event const> events() const
    {
        return { m_events.data(), m_events.size() };
    }

    void clear() { m_events.clear(); }

private:

    std::vector<Event> m_events;
};

} // namespace world

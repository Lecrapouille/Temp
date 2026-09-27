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

#include "Compages/Scene/EntityId.hpp"

#include <span>
#include <vector>

namespace scene
{

// ****************************************************************************
//! \brief What happened this frame.
// ****************************************************************************
enum class EventKind
{
    Collision,
    Trigger,
};

// ****************************************************************************
//! \brief A lightweight event emitted by physics or gameplay systems.
// ****************************************************************************
struct Event
{
    EventKind kind = EventKind::Collision;
    EntityId a{};
    EntityId b{};
};

// ****************************************************************************
//! \brief FIFO queue cleared once per frame after dispatch.
// ****************************************************************************
class EventQueue
{
public:

    void push(Event p_event) { m_events.emplace_back(p_event); }

    [[nodiscard]] std::span<Event const> events() const
    {
        return { m_events.data(), m_events.size() };
    }

    void clear() { m_events.clear(); }

private:

    std::vector<Event> m_events;
};

} // namespace scene

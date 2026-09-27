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

#include "Compages/Core/Vector.hpp"

#include <array>
#include <cstdint>

// ****************************************************************************
//! \file
//! \brief What one frame is: its size, its time and what the user did.
//!
//! Nothing here knows about a window library. The application fills a Frame
//! from GLFW, SDL, a test or a recording, and everything else reads it: the
//! behaviors of the World, the camera controllers, the Scene when it draws.
// ****************************************************************************

namespace scene
{

// ****************************************************************************
//! \brief Logical keys the examples read, independent of a physical keyboard.
// ****************************************************************************
enum class Key : std::uint8_t
{
    W,
    A,
    S,
    D,
    Q,
    E,
    Shift,
    Space,
    D1,
    D2,
    D3,
    R,
    F,
    L,
    Up,
    Down,
    Left,
    Right,
    Count
};

// ****************************************************************************
//! \brief The mouse and the keys, as they were during one frame.
// ****************************************************************************
struct Input
{
    //! \brief Where the mouse is, in pixels from the bottom left corner of
    //! the picture, the way the GPU counts.
    Vector2f mouse{ 0.0f, 0.0f };
    //! \brief How far the mouse moved during the frame, y up.
    Vector2f mouse_delta{ 0.0f, 0.0f };
    //! \brief The wheel during the frame. Positive is away from the user.
    float scroll = 0.0f;
    //! \brief The mouse is over the picture rather than over a panel or
    //! outside the window. The buttons, the motion and the wheel only count
    //! when it is.
    bool mouse_over = false;
    bool mouse_left = false;
    bool mouse_right = false;
    //! \brief The left button went down during this frame: a click.
    bool mouse_left_pressed = false;
    //! \brief The pointer is hidden and held by the picture: every motion is
    //! meant to turn a view, the way a first person game looks around.
    bool mouse_captured = false;

    //! \brief Whether each logical key was held this frame.
    std::array<bool, static_cast<std::size_t>(Key::Count)> keys{};

    [[nodiscard]] bool down(Key p_key) const
    {
        return keys[static_cast<std::size_t>(p_key)];
    }

    void set(Key p_key, bool p_value)
    {
        keys[static_cast<std::size_t>(p_key)] = p_value;
    }
};

//! \brief True when \c p_key went down between \c p_before and \c p_now.
[[nodiscard]] inline bool pressed(Input const& p_now, Input const& p_before, Key p_key)
{
    return p_now.down(p_key) && !p_before.down(p_key);
}

// ****************************************************************************
//! \brief Maps each logical key to a window-library key code (GLFW in the
//! gallery). An example may override the defaults from Try it.
// ****************************************************************************
struct KeyMap
{
    //! \brief One native key code per \c Key, in \c Key order.
    std::array<int, static_cast<std::size_t>(Key::Count)> codes{};

    //! \brief W A S D, arrows, and the keys the gallery examples use. The
    //! numbers are GLFW key codes so the header stays free of GLFW.
    [[nodiscard]] static KeyMap defaults()
    {
        KeyMap map;
        map.codes[static_cast<std::size_t>(Key::W)] = 87;
        map.codes[static_cast<std::size_t>(Key::A)] = 65;
        map.codes[static_cast<std::size_t>(Key::S)] = 83;
        map.codes[static_cast<std::size_t>(Key::D)] = 68;
        map.codes[static_cast<std::size_t>(Key::Q)] = 81;
        map.codes[static_cast<std::size_t>(Key::E)] = 69;
        map.codes[static_cast<std::size_t>(Key::Shift)] = 340;
        map.codes[static_cast<std::size_t>(Key::Space)] = 32;
        map.codes[static_cast<std::size_t>(Key::D1)] = 49;
        map.codes[static_cast<std::size_t>(Key::D2)] = 50;
        map.codes[static_cast<std::size_t>(Key::D3)] = 51;
        map.codes[static_cast<std::size_t>(Key::R)] = 82;
        map.codes[static_cast<std::size_t>(Key::F)] = 70;
        map.codes[static_cast<std::size_t>(Key::L)] = 76;
        map.codes[static_cast<std::size_t>(Key::Up)] = 265;
        map.codes[static_cast<std::size_t>(Key::Down)] = 264;
        map.codes[static_cast<std::size_t>(Key::Left)] = 263;
        map.codes[static_cast<std::size_t>(Key::Right)] = 262;
        return map;
    }

    //! \brief Fill \c p_input from \c p_key_down, which returns true while
    //! the native code is held.
    template<typename KeyDownFn>
    void apply(Input& p_input, KeyDownFn&& p_key_down) const
    {
        for (std::size_t i = 0u; i < static_cast<std::size_t>(Key::Count); ++i)
        {
            const Key key = static_cast<Key>(i);
            bool held = p_key_down(codes[i]);
            if (key == Key::Shift)
            {
                // Either shift key still counts as boost when Shift is the default bind.
                held = held || p_key_down(340) || p_key_down(344);
            }
            p_input.set(key, held);
        }
    }
};

// ****************************************************************************
//! \brief One frame: how large the picture is, how much time passed, and the
//! input.
//!
//! \code
//! void draw(scene::Frame const& p_frame)
//! {
//!     m_cube.rotate(p_frame.elapsed, { 0, 1, 0 });   // one radian a second
//!     m_scene.draw(p_frame);
//! }
//! \endcode
// ****************************************************************************
struct Frame
{
    //! \brief How wide the picture is, in pixels. It changes when the window
    //! is resized, so a projection has to read it every frame.
    std::uint32_t width = 0u;
    //! \brief How tall the picture is, in pixels.
    std::uint32_t height = 0u;
    //! \brief How long the previous frame took, in seconds: what a speed is
    //! multiplied by so that motion does not depend on the frame rate.
    float elapsed = 0.0f;
    //! \brief How long since the beginning, in seconds.
    float total = 0.0f;
    //! \brief The mouse and the keys.
    Input input{};

    //! \brief How much wider than tall the picture is: what a projection
    //! needs so that a square stays square.
    [[nodiscard]] float aspect() const
    {
        return (height == 0u) ? 1.0f : float(width) / float(height);
    }

    //! \brief Where the mouse is, in the clip space a vertex shader works
    //! in: minus one on the left and bottom edges, plus one on the others.
    [[nodiscard]] Vector2f mouseInClipSpace() const
    {
        if ((width == 0u) || (height == 0u))
        {
            return Vector2f(0.0f, 0.0f);
        }
        return Vector2f(((input.mouse.x / float(width)) * 2.0f) - 1.0f,
                        ((input.mouse.y / float(height)) * 2.0f) - 1.0f);
    }
};

} // namespace scene

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

#include "GPU/GPU.hpp"

#include <memory>
#include <string>

// ****************************************************************************
//! \file
//! \brief What every example is, seen from the gallery.
//!
//! Four things and no more: a name, a sentence saying what it demonstrates, a
//! way to set itself up, and a way to draw one frame. Everything else, the
//! window, the loop, the timing, the overlay, belongs to the gallery.
//!
//! There is deliberately no tear down. An example holds its resources as members,
//! and destroying it releases them, which is the same rule as everywhere else in
//! the library. That is also what makes the gallery a leak test: if closing an
//! example does not bring the resource counters back to where they were, the
//! example is holding something it should not, and the overlay shows it
//! immediately.
// ****************************************************************************

namespace examples
{

// ****************************************************************************
//! \brief What the gallery tells an example about the frame it is drawing.
// ****************************************************************************
struct Frame
{
    //! \brief How wide the window is now, in pixels. Changes when the window is
    //! resized, so an example computing a projection has to read it every frame
    //! rather than once.
    std::uint32_t width = 0u;
    //! \brief How tall the window is now.
    std::uint32_t height = 0u;
    //! \brief How long the previous frame took, in seconds. What to multiply a
    //! speed by so that an animation runs at the same rate whatever the frame
    //! rate.
    float elapsed = 0.0f;
    //! \brief How long since this example was set up, in seconds. Reset when the
    //! example is switched to, so an animation always starts from the beginning.
    float total = 0.0f;

    //! \brief Where the mouse is, in pixels from the bottom left corner, counted
    //! the way the graphics API counts rather than the way the window system does.
    Vector2f mouse{ 0.0f, 0.0f };
    //! \brief Mouse motion this frame, y up. Zero when the overlay ate the
    //! pointer.
    Vector2f mouse_delta{ 0.0f, 0.0f };
    //! \brief Scroll wheel this frame. Positive is away from the user.
    float scroll = 0.0f;
    bool mouse_left = false;
    bool mouse_right = false;
    //! \brief Rising edge of the left button, for a click-to-pick.
    bool mouse_left_pressed = false;
    bool key_w = false;
    bool key_a = false;
    bool key_s = false;
    bool key_d = false;
    bool key_q = false;
    bool key_e = false;
    bool key_shift = false;
    bool key_1 = false;
    bool key_2 = false;
    bool key_3 = false;

    // ------------------------------------------------------------------------
    //! \brief How much wider than tall the window is. What a projection needs so
    //! that a square stays square.
    // ------------------------------------------------------------------------
    [[nodiscard]] float aspect() const
    {
        return (height == 0u) ? 1.0f
                              : static_cast<float>(width) /
                                    static_cast<float>(height);
    }

    // ------------------------------------------------------------------------
    //! \brief Where the mouse is, as the clip space the vertex shader works in:
    //! minus one at the left and bottom edges, plus one at the right and top.
    // ------------------------------------------------------------------------
    [[nodiscard]] Vector2f mouseInClipSpace() const
    {
        if ((width == 0u) || (height == 0u))
        {
            return Vector2f(0.0f, 0.0f);
        }
        return Vector2f(
            ((mouse.x / static_cast<float>(width)) * 2.0f) - 1.0f,
            ((mouse.y / static_cast<float>(height)) * 2.0f) - 1.0f);
    }
};

// ****************************************************************************
//! \brief One demonstration, of one idea.
//!
//! Each example demonstrates a point about the design rather than a function of
//! the graphics API, which is why the description is part of the interface: the
//! gallery shows it, and writing it is how the author of an example checks it has
//! a point.
// ****************************************************************************
class Example
{
public:

    virtual ~Example() = default;

    // ------------------------------------------------------------------------
    //! \brief What this example is called, as the gallery lists it.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual std::string name() const = 0;

    // ------------------------------------------------------------------------
    //! \brief What it demonstrates, in a sentence or two the gallery shows next
    //! to it.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual std::string description() const = 0;

    // ------------------------------------------------------------------------
    //! \brief Build everything this example needs on the device.
    //!
    //! Called once, on a live device, just after the example is chosen. Anything
    //! that can fail belongs here rather than in the constructor, so that the
    //! reason reaches the gallery and appears on the screen instead of being
    //! thrown.
    //!
    //! \return why the example cannot run.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual gpu::Status setUp() = 0;

    // ------------------------------------------------------------------------
    //! \brief Draw one frame.
    //!
    //! \param[in] p_frame the size of the window and how much time has passed.
    //! \return why the frame could not be drawn. The gallery stops the example
    //! and shows the reason rather than repeating the same failure sixty times a
    //! second.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual gpu::Status draw(Frame const& p_frame) = 0;
};

//! \brief How an example is handed around.
using ExamplePtr = std::unique_ptr<Example>;

} // namespace examples

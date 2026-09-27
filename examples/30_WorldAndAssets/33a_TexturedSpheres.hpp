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

#include "Common/Example.hpp"

#include "Compages/Scene/Controls.hpp"
#include "Compages/Scene/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Spheres wearing a colour or a picture.
//!
//! A look is given when the shape is made. A picture is loaded once, however
//! many shapes wear it, and a colour given with it tints it:
//! \code
//! m_scene.sphere("Red", scene::color(0.85f, 0.25f, 0.2f));
//! m_scene.sphere("Grass", scene::texture(dataPath("grassFlowers.png")));
//! \endcode
// ****************************************************************************
class TexturedSpheres final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "33a_TexturedSpheres";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    scene::World m_world;
    scene::Scene m_scene{ m_world };
    std::vector<scene::Entity> m_spheres;
};

} // namespace examples

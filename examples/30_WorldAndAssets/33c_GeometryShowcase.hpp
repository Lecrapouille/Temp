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
//! \brief Every built-in shape, and every built-in look, on one plateau.
//!
//! The shapes are one call each and all stand one unit tall; the looks are a
//! lit colour, a colour through the textured shader, the depth and the
//! normals:
//! \code
//! m_scene.cone("cone", scene::color(0.92f, 0.55f, 0.18f));
//! m_scene.box("depth", scene::depth(8.0f, 22.0f));
//! m_scene.sphere("normals", scene::normals());
//! \endcode
//!
//! A mesh the Scene has no shortcut for, a tube here, is built by a make
//! function and given to mesh().
// ****************************************************************************
class GeometryShowcase final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "33c_GeometryShowcase";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    scene::World m_world;
    scene::Scene m_scene{ m_world };
    std::vector<scene::Entity> m_props;
};

} // namespace examples

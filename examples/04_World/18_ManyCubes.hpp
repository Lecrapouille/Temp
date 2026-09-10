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

#include "Common/Example.hpp"

#include "Assets/AssetIds.hpp"
#include "Assets/AssetManager.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <array>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A field of cubes on a slow-orbiting camera, painted in five colours.
//!
//! Two ideas that 17_MovingRobot did not exercise:
//!
//! - **Sharing at scale.** One MeshAsset (the cube) and one Material (the lit
//!   pipeline) serve every entity. Only the MaterialInstance changes from one
//!   entity to the next, so all N entities in the World cost one mesh and one
//!   program on the device, plus five parameter sets.
//!
//! - **Sort by material.** The Renderer's RenderQueue key is
//!   (material_instance, mesh), so cubes of the same colour are drawn
//!   together. Colour assignment is deliberately shuffled at authoring time
//!   (every fifth cube is red, the next is blue, ...) so the difference the
//!   sort makes is visible: the queue is not in the order the cubes were
//!   created.
//!
//! The camera moves so a fraction of the cubes fall outside the view frustum
//! every frame. The Extractor's culling drops them from the snapshot, and the
//! overlay's draw-call counter shows the number of survivors, not the total
//! in the World.
// ****************************************************************************
class ManyCubes: public Example
{
public:

    ManyCubes() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "18_ManyCubes";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    static constexpr std::size_t GRID = 12u;                     // per axis
    static constexpr std::size_t CUBE_COUNT = GRID * GRID * GRID; // 1728
    static constexpr std::size_t PALETTE_SIZE = 5u;

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::MeshAssetId m_cube_mesh;
    assets::MaterialId m_lit_material;
    std::array<assets::MaterialInstanceId, PALETTE_SIZE> m_palette{};

    world::Entity m_camera;
    world::Entity m_sun;
    world::Entity m_root;
    //! \brief Kept only to update them if a future revision animates the
    //! cubes; the frame itself only touches the camera.
    std::vector<world::Entity> m_cubes;
};

} // namespace examples

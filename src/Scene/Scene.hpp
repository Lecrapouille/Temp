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

#include "Scene/Environment.hpp"
#include "Scene/RenderSettings.hpp"
#include "World/Entity.hpp"

namespace world
{
class World;
}

namespace assets
{
class AssetManager;
}

namespace scene
{

// ****************************************************************************
//! \brief A presentation context for a World.
//!
//! A Scene is not the World. It does not own the Entities, does not own the
//! components, does not compute the transforms and does not run the systems.
//! It selects and configures what is shown for a given view:
//! - which World is observed;
//! - which AssetManager the renderer resolves ids against;
//! - which Camera Entity is used;
//! - the presentation knobs (clear colour, culling flag);
//! - the environment (ambient, fallback light direction).
//!
//! Two Scenes may reference the same World with different cameras, different
//! render settings and different environments: player view, minimap, editor
//! preview, off-screen capture.
//!
//! The Scene does not own the World or the AssetManager. Their lifetimes
//! outlive the Scene; the application makes sure of that.
// ****************************************************************************
class Scene
{
public:

    // ------------------------------------------------------------------------
    //! \brief Build a Scene observing the given World and using the given
    //! AssetManager.
    // ------------------------------------------------------------------------
    Scene(world::World& p_world, assets::AssetManager& p_assets)
        : m_world(p_world), m_assets(p_assets)
    {
    }

    Scene(Scene const&) = delete;
    Scene& operator=(Scene const&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;

    // ------------------------------------------------------------------------
    //! \brief The World this Scene observes.
    // ------------------------------------------------------------------------
    [[nodiscard]] world::World& world()
    {
        return m_world;
    }
    [[nodiscard]] world::World const& world() const
    {
        return m_world;
    }

    // ------------------------------------------------------------------------
    //! \brief The AssetManager the renderer resolves ids against.
    // ------------------------------------------------------------------------
    [[nodiscard]] assets::AssetManager& assets()
    {
        return m_assets;
    }
    [[nodiscard]] assets::AssetManager const& assets() const
    {
        return m_assets;
    }

    // ------------------------------------------------------------------------
    //! \brief Which Camera Entity is used for the primary view.
    // ------------------------------------------------------------------------
    void setActiveCamera(world::Entity p_camera)
    {
        m_active_camera = p_camera;
    }
    [[nodiscard]] world::Entity activeCamera() const
    {
        return m_active_camera;
    }

    // ------------------------------------------------------------------------
    //! \brief The presentation knobs.
    // ------------------------------------------------------------------------
    [[nodiscard]] RenderSettings& renderSettings()
    {
        return m_settings;
    }
    [[nodiscard]] RenderSettings const& renderSettings() const
    {
        return m_settings;
    }

    // ------------------------------------------------------------------------
    //! \brief The environment.
    // ------------------------------------------------------------------------
    [[nodiscard]] Environment& environment()
    {
        return m_environment;
    }
    [[nodiscard]] Environment const& environment() const
    {
        return m_environment;
    }

private:

    world::World& m_world;
    assets::AssetManager& m_assets;
    world::Entity m_active_camera{};
    RenderSettings m_settings{};
    Environment m_environment{};
};

} // namespace scene

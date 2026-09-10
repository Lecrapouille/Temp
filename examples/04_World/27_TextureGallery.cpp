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

#include "04_World/27_TextureGallery.hpp"

#include "Assets/Primitives.hpp"
#include "Assets/TextureAsset.hpp"
#include "Common/DataPath.hpp"
#include "GPU/RenderPass.hpp"
#include "GPU/Texture.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <array>
#include <cmath>
#include <string>

using namespace units::literals;

namespace examples
{

namespace
{

struct GalleryEntry
{
    char const* file;
    char const* label;
    Vector3f fallback_color;
};

const std::array<GalleryEntry, 9u> kGallery{ {
    { "grassFlowers.png", "grass", Vector3f(0.45f, 0.75f, 0.35f) },
    { "rocks.png", "rocks", Vector3f(0.55f, 0.52f, 0.48f) },
    { "mud.png", "mud", Vector3f(0.45f, 0.32f, 0.18f) },
    { "grassy2.png", "grassy", Vector3f(0.35f, 0.65f, 0.30f) },
    { "wooden-crate.jpg", "crate", Vector3f(0.62f, 0.42f, 0.22f) },
    { "cowboy.png", "cowboy", Vector3f(0.70f, 0.55f, 0.40f) },
    { "fields.png", "fields", Vector3f(0.50f, 0.70f, 0.25f) },
    { "hazard.png", "hazard", Vector3f(0.85f, 0.75f, 0.15f) },
    { "tree-01.png", "tree", Vector3f(0.30f, 0.55f, 0.25f) },
} };

} // namespace

//------------------------------------------------------------------------------
std::string TextureGallery::description() const
{
    return "Textures from the legacy OpenGLCppWrapper-data repository on a "
           "3x3 grid of UV spheres. Each PNG or JPG becomes a TextureAsset; "
           "missing files fall back to a flat baseColorFactor so --check still "
           "passes when only part of the data checkout is present.";
}

//------------------------------------------------------------------------------
gpu::Status TextureGallery::setUp()
{
    GPU_TRY_ASSIGN(sphere_mesh, assets::makeSphere(0.55f, 24u, 32u));
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("sphere", std::move(sphere_mesh)));
    m_sphere_mesh = mesh_id;

    GPU_TRY_ASSIGN(pbr, assets::makePbrMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("pbr", std::move(pbr)));
    m_pbr_material = material_id;

    world::Entity root = m_world.create("Root");
    m_spheres.reserve(kGallery.size());

    for (std::size_t i = 0u; i < kGallery.size(); ++i)
    {
        GalleryEntry const& entry = kGallery[i];
        assets::MaterialInstance instance;
        instance.material = m_pbr_material;
        instance.base_color_factor = entry.fallback_color;

        const std::string path = dataPath(entry.file);
        if (!path.empty())
        {
            gpu::LoadOptions options;
            options.srgb = true;
            GPU_TRY_ASSIGN(gpu_texture,
                           gpu::Texture::fromFile(path, options));
            assets::TextureAsset texture;
            texture.name = entry.label;
            texture.texture = std::move(gpu_texture);
            GPU_TRY_ASSIGN(texture_id,
                           m_assets.addTexture(texture.name, std::move(texture)));
            instance.base_color_texture = texture_id;
        }

        GPU_TRY_ASSIGN(
            instance_id,
            m_assets.addMaterialInstance(entry.label, std::move(instance)));

        const std::size_t col = i % 3u;
        const std::size_t row = i / 3u;
        world::Entity entity = m_world.create(entry.label);
        m_world.transform(entity).position =
            Vector3f((static_cast<float>(col) - 1.0f) * 3.0f,
                     0.6f,
                     (static_cast<float>(row) - 1.0f) * 3.0f);
        m_world.add(entity,
                    world::MeshRenderer{ m_sphere_mesh, instance_id });
        GPU_TRY(m_world.setParent(entity, root));
        m_spheres.push_back(entity);
    }

    world::Entity sun = m_world.create("Sun");
    m_world.transform(sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.7),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.96f, 0.90f), 1.0f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 50.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 200.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 3.5f, 10.0f);
    const Quatf pitch = Quatf::fromAngleAxis(units::angle::radian_t(-0.35),
                                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_camera).rotation = pitch;

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.08f, 0.10f, 0.14f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.16f, 0.16f, 0.18f);

    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status TextureGallery::draw(Frame const& p_frame)
{
    const float t = p_frame.total * 0.35f;
    const float radius = 10.5f;
    m_world.transform(m_camera).position =
        Vector3f(radius * std::sin(t), 3.5f, radius * std::cos(t));
    const Quatf yaw = Quatf::fromAngleAxis(units::angle::radian_t(t),
                                           Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.35), Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_camera).rotation = yaw * pitch;

    m_world.update();

    GPU_TRY_ASSIGN(
        snapshot,
        render::Extractor::extract(m_scene, p_frame.width, p_frame.height));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = m_scene.renderSettings().clear_color;
    desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    return m_renderer.render(pass, snapshot, m_assets);
}

} // namespace examples

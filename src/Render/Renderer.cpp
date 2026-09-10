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

#include "Render/Renderer.hpp"

#include "Assets/AssetManager.hpp"
#include "Assets/ShaderLib.hpp"
#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace render
{

namespace
{

constexpr std::array<std::string_view, assets::shaders::MAX_POINT_LIGHTS>
    kPointPosNames{
        "pointLightPos0", "pointLightPos1", "pointLightPos2", "pointLightPos3"
    };
constexpr std::array<std::string_view, assets::shaders::MAX_POINT_LIGHTS>
    kPointColorNames{ "pointLightColor0",
                      "pointLightColor1",
                      "pointLightColor2",
                      "pointLightColor3" };
constexpr std::array<std::string_view, assets::shaders::MAX_POINT_LIGHTS>
    kPointRangeNames{ "pointLightRange0",
                      "pointLightRange1",
                      "pointLightRange2",
                      "pointLightRange3" };

gloop::Status bindCamera(assets::Material& p_material,
                       RenderSnapshot const& p_snapshot)
{
    GPU_TRY(p_material.program.set("view", p_snapshot.camera.view));
    GPU_TRY(p_material.program.set("projection",
                                   p_snapshot.camera.projection));
    return gloop::success();
}

gloop::Status bindLighting(assets::Material& p_material,
                         RenderSnapshot const& p_snapshot)
{
    GPU_TRY(bindCamera(p_material, p_snapshot));

    Vector3f light_direction = p_snapshot.environment.default_light_direction;
    if (!p_snapshot.directional_lights.empty())
    {
        light_direction = p_snapshot.directional_lights.front().direction;
    }

    GPU_TRY(p_material.program.set("lightDir", light_direction));
    GPU_TRY(p_material.program.set("ambient", p_snapshot.environment.ambient));

    const int count = static_cast<int>(std::min(
        p_snapshot.point_lights.size(),
        static_cast<std::size_t>(assets::shaders::MAX_POINT_LIGHTS)));
    GPU_TRY(p_material.program.set("pointLightCount", count));

    for (int i = 0; i < assets::shaders::MAX_POINT_LIGHTS; ++i)
    {
        if (i < count)
        {
            PointLightFrame const& light =
                p_snapshot.point_lights[static_cast<std::size_t>(i)];
            GPU_TRY(p_material.program.set(kPointPosNames[static_cast<std::size_t>(i)],
                                           light.position));
            GPU_TRY(p_material.program.set(
                kPointColorNames[static_cast<std::size_t>(i)],
                light.color * light.intensity));
            GPU_TRY(p_material.program.set(
                kPointRangeNames[static_cast<std::size_t>(i)], light.range));
        }
        else
        {
            GPU_TRY(p_material.program.set(
                kPointPosNames[static_cast<std::size_t>(i)],
                Vector3f(0.0f, 0.0f, 0.0f)));
            GPU_TRY(p_material.program.set(
                kPointColorNames[static_cast<std::size_t>(i)],
                Vector3f(0.0f, 0.0f, 0.0f)));
            GPU_TRY(
                p_material.program.set(kPointRangeNames[static_cast<std::size_t>(i)],
                                       1.0f));
        }
    }
    return gloop::success();
}

gloop::Status bindMaterialFrame(assets::Material& p_material,
                              RenderSnapshot const& p_snapshot)
{
    switch (p_material.family)
    {
        case assets::ShaderFamily::Lit:
        case assets::ShaderFamily::PbrMinimal:
            return bindLighting(p_material, p_snapshot);
        case assets::ShaderFamily::Depth:
        case assets::ShaderFamily::Normals:
        default:
            return bindCamera(p_material, p_snapshot);
    }
}

} // namespace

//------------------------------------------------------------------------------
gloop::Status Renderer::render(gpu::RenderPass const& /*p_pass*/,
                             RenderSnapshot const& p_snapshot,
                             assets::AssetManager& p_assets)
{
    if (!gpu::inRenderPass())
    {
        return gloop::failure(
            "Renderer::render was called with no pass open. The pass says "
            "where to draw and what the target starts from");
    }

    m_queue.build(p_snapshot, p_assets);
    if (m_queue.empty())
    {
        return gloop::success();
    }

    assets::MaterialId bound_material;
    assets::Material* material = nullptr;

    for (QueueEntry const& entry : m_queue.entries())
    {
        RenderItem const& item = p_snapshot.items[entry.item_index];

        assets::MaterialInstance const* instance =
            p_assets.materialInstance(item.material_instance);
        if (instance == nullptr)
        {
            continue;
        }

        if (!(bound_material == instance->material))
        {
            material = p_assets.material(instance->material);
            if (material == nullptr)
            {
                continue;
            }
            bound_material = instance->material;
            GPU_TRY(bindMaterialFrame(*material, p_snapshot));
        }

        if (material == nullptr)
        {
            continue;
        }

        assets::MeshAsset const* mesh = p_assets.mesh(item.mesh);
        if (mesh == nullptr)
        {
            continue;
        }

        GPU_TRY(material->program.set("model", item.world_matrix));
        switch (material->family)
        {
            case assets::ShaderFamily::PbrMinimal:
            {
                GPU_TRY(material->program.set("baseColorFactor",
                                              instance->base_color_factor));
                const bool has_map = instance->base_color_texture.valid();
                GPU_TRY(material->program.set("hasBaseColorMap", has_map));
                if (has_map)
                {
                    assets::TextureAsset const* texture =
                        p_assets.texture(instance->base_color_texture);
                    if (texture != nullptr)
                    {
                        GPU_TRY(texture->texture.bind(0u));
                        GPU_TRY(material->program.set("baseColorMap", 0));
                    }
                }
                break;
            }
            case assets::ShaderFamily::Depth:
                GPU_TRY(material->program.set("near", instance->depth_near));
                GPU_TRY(material->program.set("far", instance->depth_far));
                GPU_TRY(material->program.set("opacity", instance->opacity));
                break;
            case assets::ShaderFamily::Normals:
                GPU_TRY(material->program.set("opacity", instance->opacity));
                break;
            case assets::ShaderFamily::Lit:
            default:
                GPU_TRY(material->program.set("color", instance->color));
                break;
        }
        GPU_TRY(gpu::drawIndexed(material->pipeline,
                                 mesh->vertices.handle(),
                                 mesh->indices.handle(),
                                 mesh->index_type,
                                 mesh->index_count));
    }
    return gloop::success();
}

} // namespace render

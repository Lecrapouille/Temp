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

#include "Compages/Scene/SceneSerializer.hpp"

#include "Compages/Scene/Assets/AssetManager.hpp"
#include "Compages/Scene/Scene.hpp"
#include "Compages/Scene/Camera.hpp"
#include "Compages/Scene/Light.hpp"
#include "Compages/Scene/MeshRenderer.hpp"
#include "Compages/Scene/PrefabInstance.hpp"
#include "Compages/Scene/World.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <unordered_map>
#include <vector>

namespace scene
{

namespace
{

using json = nlohmann::json;

json vec3(Vector3f const& p_v)
{
    return json::array({ p_v.x, p_v.y, p_v.z });
}

json quat(Quatf const& p_q)
{
    return json::array({ p_q[0], p_q[1], p_q[2], p_q[3] });
}

Vector3f readVec3(json const& p_j)
{
    return Vector3f(p_j.at(0).get<float>(),
                    p_j.at(1).get<float>(),
                    p_j.at(2).get<float>());
}

Quatf readQuat(json const& p_j)
{
    return Quatf(p_j.at(0).get<float>(),
                 p_j.at(1).get<float>(),
                 p_j.at(2).get<float>(),
                 p_j.at(3).get<float>());
}

void collectSubtree(scene::World const& p_world,
                    scene::EntityId p_entity,
                    std::vector<scene::EntityId>& p_order,
                    std::unordered_map<scene::EntityId, int>& p_index)
{
    p_index[p_entity] = static_cast<int>(p_order.size());
    p_order.emplace_back(p_entity);
    scene::EntityId child = p_world.firstChild(p_entity);
    while (child.valid())
    {
        collectSubtree(p_world, child, p_order, p_index);
        child = p_world.nextSibling(child);
    }
}

json serializeEntity(scene::World const& p_world,
                     scene::AssetManager const& p_assets,
                     scene::EntityId p_entity,
                     int p_parent_index)
{
    json node;
    node["name"] = p_world.name(p_entity);
    node["parent"] = p_parent_index;
    node["enabled"] = p_world.enabled(p_entity);

    const scene::LocalTransform& tr = p_world.transform(p_entity);
    node["transform"] = { { "position", vec3(tr.position) },
                          { "rotation", quat(tr.rotation) },
                          { "scale", vec3(tr.scale) } };

    if (scene::MeshRenderer const* renderer =
            p_world.tryGet<scene::MeshRenderer>(p_entity))
    {
        node["mesh_renderer"] = {
            { "mesh", p_assets.meshName(renderer->mesh) },
            { "material_instance",
              p_assets.materialInstanceName(renderer->material_instance) },
            { "flags",
              static_cast<std::uint32_t>(renderer->flags) }
        };
    }

    if (scene::Camera const* camera = p_world.tryGet<scene::Camera>(p_entity))
    {
        node["camera"] = {
            { "projection",
              (camera->projection == scene::Projection::Perspective)
                  ? "perspective"
                  : "orthographic" },
            { "fov_degrees", camera->fov_degrees },
            { "ortho_half_height", camera->ortho_half_height },
            { "near_plane", camera->near_plane },
            { "far_plane", camera->far_plane }
        };
    }

    if (scene::DirectionalLight const* light =
            p_world.tryGet<scene::DirectionalLight>(p_entity))
    {
        node["directional_light"] = {
            { "color", vec3(light->color) },
            { "intensity", light->intensity }
        };
    }

    if (scene::PointLight const* point =
            p_world.tryGet<scene::PointLight>(p_entity))
    {
        node["point_light"] = {
            { "color", vec3(point->color) },
            { "intensity", point->intensity },
            { "range", point->range }
        };
    }

    if (scene::PrefabInstance const* prefab =
            p_world.tryGet<scene::PrefabInstance>(p_entity))
    {
        node["prefab_instance"] = {
            { "prefab", p_assets.prefabName(prefab->prefab) }
        };
    }

    return node;
}

compages::Status applyComponents(scene::World& p_world,
                            scene::AssetManager const& p_assets,
                            scene::EntityId p_entity,
                            json const& p_node)
{
    if (p_node.contains("mesh_renderer"))
    {
        json const& mr = p_node.at("mesh_renderer");
        const std::string mesh_name = mr.at("mesh").get<std::string>();
        const std::string material_name =
            mr.at("material_instance").get<std::string>();
        const scene::MeshAssetId mesh = p_assets.findMesh(mesh_name);
        const scene::MaterialInstanceId material =
            p_assets.findMaterialInstance(material_name);
        if (!mesh.valid() || !material.valid())
        {
            return compages::failure("scene file references missing assets");
        }
        scene::MeshRenderer renderer;
        renderer.mesh = mesh;
        renderer.material_instance = material;
        renderer.flags = static_cast<scene::RenderFlags>(
            mr.at("flags").get<std::uint32_t>());
        p_world.add(p_entity, renderer);
    }

    if (p_node.contains("camera"))
    {
        json const& cam = p_node.at("camera");
        scene::Camera camera;
        const std::string projection = cam.at("projection").get<std::string>();
        camera.projection = (projection == "orthographic")
                                ? scene::Projection::Orthographic
                                : scene::Projection::Perspective;
        camera.fov_degrees = cam.at("fov_degrees").get<float>();
        camera.ortho_half_height = cam.at("ortho_half_height").get<float>();
        camera.near_plane = cam.at("near_plane").get<float>();
        camera.far_plane = cam.at("far_plane").get<float>();
        p_world.add(p_entity, camera);
    }

    if (p_node.contains("directional_light"))
    {
        json const& light = p_node.at("directional_light");
        scene::DirectionalLight directional;
        directional.color = readVec3(light.at("color"));
        directional.intensity = light.at("intensity").get<float>();
        p_world.add(p_entity, directional);
    }

    if (p_node.contains("point_light"))
    {
        json const& light = p_node.at("point_light");
        scene::PointLight point;
        point.color = readVec3(light.at("color"));
        point.intensity = light.at("intensity").get<float>();
        point.range = light.value("range", 100.0f);
        p_world.add(p_entity, point);
    }

    if (p_node.contains("prefab_instance"))
    {
        const std::string prefab_name =
            p_node.at("prefab_instance").at("prefab").get<std::string>();
        const scene::PrefabId prefab = p_assets.findPrefab(prefab_name);
        if (!prefab.valid())
        {
            return compages::failure("scene file references unknown prefab '" +
                               prefab_name + "'");
        }
        p_world.add(p_entity, scene::PrefabInstance{ prefab });
    }

    return compages::success();
}

} // namespace

//------------------------------------------------------------------------------
compages::Status save(scene::World const& p_world,
                 scene::AssetManager const& p_assets,
                 std::string const& p_path)
{
    std::vector<scene::EntityId> order;
    std::unordered_map<scene::EntityId, int> index;
    p_world.spatial().forEachRoot([&](scene::NodeId p_root) {
        collectSubtree(p_world,
                       p_world.spatial().entityOf(p_root),
                       order,
                       index);
    });

    json document;
    document["version"] = 1;
    json entities = json::array();
    for (scene::EntityId entity : order)
    {
        scene::EntityId parent = p_world.parent(entity);
        const int parent_index =
            parent.valid() ? index.at(parent) : -1;
        entities.emplace_back(
            serializeEntity(p_world, p_assets, entity, parent_index));
    }
    document["entities"] = std::move(entities);

    std::ofstream out(p_path);
    if (!out)
    {
        return compages::failure("cannot write scene file '" + p_path + "'");
    }
    out << document.dump(2);
    return compages::success();
}

//------------------------------------------------------------------------------
compages::Result<std::vector<scene::EntityId>>
load(scene::World& p_world,
     scene::AssetManager const& p_assets,
     std::string const& p_path)
{
    std::ifstream in(p_path);
    if (!in)
    {
        return compages::failure("cannot read scene file '" + p_path + "'");
    }

    json document;
    in >> document;
    if (!document.contains("entities"))
    {
        return compages::failure("scene file has no entities array");
    }

    json const& entities = document.at("entities");
    std::vector<scene::EntityId> created;
    created.reserve(entities.size());

    for (json const& node : entities)
    {
        scene::EntityId entity = p_world.create(node.value("name", ""));
        if (!node.value("enabled", true))
        {
            p_world.setEnabled(entity, false);
        }

        if (node.contains("transform"))
        {
            json const& tr = node.at("transform");
            scene::LocalTransform local;
            local.position = readVec3(tr.at("position"));
            local.rotation = readQuat(tr.at("rotation"));
            local.scale = readVec3(tr.at("scale"));
            p_world.transform(entity) = local;
        }

        COMPAGES_TRY(applyComponents(p_world, p_assets, entity, node));
        created.emplace_back(entity);
    }

    for (std::size_t i = 0u; i < entities.size(); ++i)
    {
        const int parent_index = entities.at(i).value("parent", -1);
        if (parent_index >= 0)
        {
            COMPAGES_TRY(p_world.setParent(
                created[i], created[static_cast<std::size_t>(parent_index)]));
        }
    }

    std::vector<scene::EntityId> roots;
    for (std::size_t i = 0u; i < entities.size(); ++i)
    {
        if (entities.at(i).value("parent", -1) < 0)
        {
            roots.emplace_back(created[i]);
        }
    }

    p_world.update();
    return roots;
}

//------------------------------------------------------------------------------
compages::Status saveScene(scene::Scene const& p_scene, std::string const& p_path)
{
    json document;
    document["version"] = 2;

    json scene_node;
    if (p_scene.activeCamera())
    {
        scene_node["active_camera"] = p_scene.world().name(p_scene.activeCamera());
    }
    scene_node["clear_color"] = json::array({
        p_scene.renderSettings().clear_color.x,
        p_scene.renderSettings().clear_color.y,
        p_scene.renderSettings().clear_color.z,
        p_scene.renderSettings().clear_color.w
    });
    scene_node["frustum_culling"] = p_scene.renderSettings().frustum_culling;
    scene_node["ambient"] =
        vec3(p_scene.environment().ambient);
    scene_node["default_light_direction"] =
        vec3(p_scene.environment().default_light_direction);
    document["scene"] = std::move(scene_node);

    std::vector<scene::EntityId> order;
    std::unordered_map<scene::EntityId, int> index;
    p_scene.world().spatial().forEachRoot([&](scene::NodeId p_root) {
        collectSubtree(p_scene.world(),
                       p_scene.world().spatial().entityOf(p_root),
                       order,
                       index);
    });

    json entities = json::array();
    for (scene::EntityId entity : order)
    {
        scene::EntityId parent = p_scene.world().parent(entity);
        const int parent_index = parent.valid() ? index.at(parent) : -1;
        entities.emplace_back(serializeEntity(
            p_scene.world(), p_scene.assets(), entity, parent_index));
    }
    document["entities"] = std::move(entities);

    std::ofstream out(p_path);
    if (!out)
    {
        return compages::failure("cannot write scene file '" + p_path + "'");
    }
    out << document.dump(2);
    return compages::success();
}

//------------------------------------------------------------------------------
compages::Result<std::vector<scene::EntityId>>
loadScene(scene::Scene& p_scene, std::string const& p_path)
{
    std::ifstream in(p_path);
    if (!in)
    {
        return compages::failure("cannot read scene file '" + p_path + "'");
    }

    json document;
    in >> document;
    if (!document.contains("entities"))
    {
        return compages::failure("scene file has no entities array");
    }

    if (document.contains("scene"))
    {
        json const& scene_node = document.at("scene");
        if (scene_node.contains("clear_color"))
        {
            json const& color = scene_node.at("clear_color");
            p_scene.renderSettings().clear_color =
                Vector4f(color.at(0).get<float>(),
                         color.at(1).get<float>(),
                         color.at(2).get<float>(),
                         color.at(3).get<float>());
        }
        p_scene.renderSettings().frustum_culling =
            scene_node.value("frustum_culling", true);
        if (scene_node.contains("ambient"))
        {
            p_scene.environment().ambient = readVec3(scene_node.at("ambient"));
        }
        if (scene_node.contains("default_light_direction"))
        {
            p_scene.environment().default_light_direction =
                readVec3(scene_node.at("default_light_direction"));
        }
    }

    scene::World& world = p_scene.world();
    json const& entities = document.at("entities");
    std::vector<scene::EntityId> created;
    created.reserve(entities.size());

    for (json const& node : entities)
    {
        scene::EntityId entity = world.create(node.value("name", ""));
        if (!node.value("enabled", true))
        {
            world.setEnabled(entity, false);
        }

        if (node.contains("transform"))
        {
            json const& tr = node.at("transform");
            scene::LocalTransform local;
            local.position = readVec3(tr.at("position"));
            local.rotation = readQuat(tr.at("rotation"));
            local.scale = readVec3(tr.at("scale"));
            world.transform(entity) = local;
        }

        COMPAGES_TRY(applyComponents(world, p_scene.assets(), entity, node));
        created.emplace_back(entity);
    }

    for (std::size_t i = 0u; i < entities.size(); ++i)
    {
        const int parent_index = entities.at(i).value("parent", -1);
        if (parent_index >= 0)
        {
            COMPAGES_TRY(world.setParent(
                created[i], created[static_cast<std::size_t>(parent_index)]));
        }
    }

    if (document.contains("scene"))
    {
        json const& scene_node = document.at("scene");
        if (scene_node.contains("active_camera"))
        {
            const std::string camera_name =
                scene_node.at("active_camera").get<std::string>();
            for (scene::EntityId entity : created)
            {
                if (world.name(entity) == camera_name)
                {
                    p_scene.activeCamera(entity);
                    break;
                }
            }
        }
    }

    std::vector<scene::EntityId> roots;
    for (std::size_t i = 0u; i < entities.size(); ++i)
    {
        if (entities.at(i).value("parent", -1) < 0)
        {
            roots.emplace_back(created[i]);
        }
    }

    world.update();
    return roots;
}

} // namespace scene

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

#include "Compages/Scene/PrefabInstantiate.hpp"

#include "Compages/Scene/Assets/AssetManager.hpp"
#include "Compages/Scene/Assets/Prefab.hpp"
#include "Compages/Scene/Camera.hpp"
#include "Compages/Scene/Animator.hpp"
#include "Compages/Scene/Light.hpp"
#include "Compages/Scene/MeshRenderer.hpp"
#include "Compages/Scene/PrefabInstance.hpp"
#include "Compages/Scene/SkinInstance.hpp"
#include "Compages/Scene/World.hpp"

#include <utility>
#include <vector>

namespace scene
{

namespace
{

compages::Status spawnNode(World& p_world,
                      scene::AssetManager& p_assets,
                      scene::PrefabId p_prefab,
                      scene::PrefabNode const& p_node,
                      EntityId p_parent,
                      EntityId& p_root_out,
                      bool p_is_root,
                      LocalTransform const& p_root_offset,
                      std::vector<EntityId>& p_nodes,
                      std::vector<std::pair<EntityId,
                          scene::PrefabSkinInstance const*>>& p_skins)
{
    EntityId entity = p_world.create(p_node.name);
    if (p_node.source_index >= p_nodes.size())
    {
        p_nodes.resize(
            static_cast<std::size_t>(p_node.source_index) + 1u);
    }
    p_nodes[p_node.source_index] = entity;
    if (p_is_root)
    {
        p_root_out = entity;
        p_world.add(entity, PrefabInstance{ p_prefab });
    }

    LocalTransform local = p_node.transform;
    if (p_is_root)
    {
        local.position = local.position + p_root_offset.position;
        local.rotation = p_root_offset.rotation * local.rotation;
        local.scale = Vector3f(local.scale.x * p_root_offset.scale.x,
                               local.scale.y * p_root_offset.scale.y,
                               local.scale.z * p_root_offset.scale.z);
    }
    p_world.transform(entity) = local;
    if (!p_node.enabled)
    {
        p_world.setEnabled(entity, false);
    }

    if (p_node.mesh_renderer.has_value())
    {
        scene::PrefabMeshRenderer const& src = *p_node.mesh_renderer;
        const scene::MeshAssetId mesh = p_assets.findMesh(src.mesh);
        const scene::MaterialInstanceId material =
            p_assets.findMaterialInstance(src.material_instance);
        if (!mesh.valid())
        {
            return compages::failure("prefab references unknown mesh '" + src.mesh +
                                "'");
        }
        if (!material.valid())
        {
            return compages::failure("prefab references unknown material instance '" +
                                src.material_instance + "'");
        }
        MeshRenderer renderer;
        renderer.mesh = mesh;
        renderer.material_instance = material;
        renderer.flags = src.flags;
        p_world.add(entity, renderer);
    }

    if (p_node.camera.has_value())
    {
        p_world.add(entity, *p_node.camera);
    }
    if (p_node.directional_light.has_value())
    {
        p_world.add(entity, *p_node.directional_light);
    }
    if (p_node.skin_instance.has_value())
    {
        p_skins.emplace_back(entity, &*p_node.skin_instance);
    }

    if (p_parent.valid())
    {
        COMPAGES_TRY(p_world.setParent(entity, p_parent));
    }

    for (scene::PrefabNode const& child : p_node.children)
    {
        COMPAGES_TRY(spawnNode(p_world,
                          p_assets,
                          p_prefab,
                          child,
                          entity,
                          p_root_out,
                          false,
                          p_root_offset,
                          p_nodes,
                          p_skins));
    }
    return compages::success();
}

} // namespace

//------------------------------------------------------------------------------
compages::Result<EntityId> instantiate(World& p_world,
                                scene::AssetManager& p_assets,
                                scene::PrefabId p_prefab,
                                EntityId p_parent,
                                LocalTransform p_root_offset)
{
    scene::Prefab const* prefab = p_assets.prefab(p_prefab);
    if (prefab == nullptr)
    {
        return compages::failure("instantiate called with a stale prefab id");
    }

    EntityId root;
    std::vector<EntityId> nodes;
    std::vector<std::pair<EntityId,
        scene::PrefabSkinInstance const*>> skins;
    COMPAGES_TRY(spawnNode(p_world,
                      p_assets,
                      p_prefab,
                      prefab->root,
                      p_parent,
                      root,
                      true,
                      p_root_offset,
                      nodes,
                      skins));
    for (auto const& [entity, source] : skins)
    {
        SkinInstance instance;
        instance.joints.reserve(source->joints.size());
        for (std::uint32_t const joint : source->joints)
        {
            instance.joints.emplace_back(
                (joint < nodes.size()) ? nodes[joint] : EntityId{});
        }
        p_world.add(entity, std::move(instance));
    }
    if (!prefab->animations.empty())
    {
        Animator animator;
        animator.clip = prefab->animations.front();
        animator.clips = prefab->animations;
        animator.targets = std::move(nodes);
        p_world.add(root, std::move(animator));
    }
    p_world.update();
    return root;
}

} // namespace scene

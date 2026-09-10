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

#include "World/PrefabInstantiate.hpp"

#include "Assets/AssetManager.hpp"
#include "Assets/Prefab.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/PrefabInstance.hpp"
#include "World/World.hpp"

namespace world
{

namespace
{

gloop::Status spawnNode(World& p_world,
                      assets::AssetManager& p_assets,
                      assets::PrefabId p_prefab,
                      assets::PrefabNode const& p_node,
                      Entity p_parent,
                      Entity& p_root_out,
                      bool p_is_root,
                      LocalTransform const& p_root_offset)
{
    Entity entity = p_world.create(p_node.name);
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
        assets::PrefabMeshRenderer const& src = *p_node.mesh_renderer;
        const assets::MeshAssetId mesh = p_assets.findMesh(src.mesh);
        const assets::MaterialInstanceId material =
            p_assets.findMaterialInstance(src.material_instance);
        if (!mesh.valid())
        {
            return gloop::failure("prefab references unknown mesh '" + src.mesh +
                                "'");
        }
        if (!material.valid())
        {
            return gloop::failure("prefab references unknown material instance '" +
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

    if (p_parent.valid())
    {
        GPU_TRY(p_world.setParent(entity, p_parent));
    }

    for (assets::PrefabNode const& child : p_node.children)
    {
        GPU_TRY(spawnNode(p_world,
                          p_assets,
                          p_prefab,
                          child,
                          entity,
                          p_root_out,
                          false,
                          p_root_offset));
    }
    return gloop::success();
}

} // namespace

//------------------------------------------------------------------------------
gloop::Result<Entity> instantiate(World& p_world,
                                assets::AssetManager& p_assets,
                                assets::PrefabId p_prefab,
                                Entity p_parent,
                                LocalTransform p_root_offset)
{
    assets::Prefab const* prefab = p_assets.prefab(p_prefab);
    if (prefab == nullptr)
    {
        return gloop::failure("instantiate called with a stale prefab id");
    }

    Entity root;
    GPU_TRY(spawnNode(p_world,
                      p_assets,
                      p_prefab,
                      prefab->root,
                      p_parent,
                      root,
                      true,
                      p_root_offset));
    return root;
}

} // namespace world

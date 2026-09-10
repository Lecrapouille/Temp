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

#include "Assets/Loaders/GltfLoader.hpp"

#include "Assets/AnimationClip.hpp"
#include "Assets/Primitives.hpp"
#include "Assets/Skin.hpp"
#include "World/Components/Animator.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/SkinInstance.hpp"

#include "GPU/Core/PixelFormat.hpp"
#include "GPU/Texture.hpp"

#include "cgltf.h"

#include <stb_image.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace assets
{

namespace
{

using TextureMap = std::unordered_map<cgltf_image*, TextureAssetId>;

[[nodiscard]] gloop::Result<gpu::Texture>
textureFromMemory(std::span<const std::byte> p_pixels, bool p_srgb)
{
    // glTF UV (0, 0) is the top-left of the image file. OpenGL treats the
    // first row in memory as V = 0. Leaving the file unflipped therefore
    // matches the accessor; flipping here (as Texture::fromFile does for
    // OpenGL-authored UVs) puts the beak and the eyes on the wrong side.
    stbi_set_flip_vertically_on_load(0);
    int width = 0;
    int height = 0;
    int channels = 0;
    // Four channels: RGB widths that are not a multiple of four would
    // otherwise trip GL_UNPACK_ALIGNMENT.
    unsigned char* decoded = stbi_load_from_memory(
        reinterpret_cast<unsigned char const*>(p_pixels.data()),
        static_cast<int>(p_pixels.size()),
        &width,
        &height,
        &channels,
        4);
    if (decoded == nullptr)
    {
        const char* why = stbi_failure_reason();
        return gloop::failure(std::string("embedded glTF image: ") +
                            ((why == nullptr) ? "unknown reason" : why));
    }

    const gpu::PixelFormat format =
        p_srgb ? gpu::PixelFormat::SRGB8A8 : gpu::PixelFormat::RGBA8;

    gpu::TextureDesc desc;
    desc.kind = gpu::TextureKind::Texture2D;
    desc.format = format;
    desc.width = static_cast<std::uint32_t>(width);
    desc.height = static_cast<std::uint32_t>(height);
    desc.levels = 0u;
    desc.wrap_x = gpu::Wrap::Repeat;
    desc.wrap_y = gpu::Wrap::Repeat;

    GPU_TRY_ASSIGN(texture, gpu::Texture::create(desc));
    const std::size_t bytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
        gpu::bytesPerPixel(format);
    GPU_TRY(texture.write(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(decoded), bytes)));
    stbi_image_free(decoded);
    GPU_TRY(texture.generateMipmaps());
    return texture;
}

[[nodiscard]] gloop::Result<TextureAssetId>
importImage(cgltf_image* p_image,
            AssetManager& p_assets,
            TextureMap& p_cache,
            std::size_t& p_texture_count)
{
    if (p_image == nullptr)
    {
        return gloop::failure("glTF image pointer is null");
    }
    const auto cached = p_cache.find(p_image);
    if (cached != p_cache.end())
    {
        return cached->second;
    }

    std::span<const std::byte> bytes;
    if ((p_image->buffer_view != nullptr) &&
        (p_image->buffer_view->buffer != nullptr))
    {
        const cgltf_buffer_view* view = p_image->buffer_view;
        bytes = std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(view->buffer->data) +
                view->offset,
            view->size);
    }
    else if (p_image->uri != nullptr)
    {
        return gloop::failure(
            "external glTF image URIs are not supported yet; embed images "
            "in the GLB");
    }
    else
    {
        return gloop::failure("glTF image has neither a buffer view nor a URI");
    }

    GPU_TRY_ASSIGN(gpu_texture, textureFromMemory(bytes, true));
    TextureAsset texture;
    texture.name = (p_image->name != nullptr) ? p_image->name : "gltf-image";
    texture.texture = std::move(gpu_texture);
    GPU_TRY_ASSIGN(id, p_assets.addTexture(texture.name, std::move(texture)));
    p_cache.emplace(p_image, id);
    ++p_texture_count;
    return id;
}

[[nodiscard]] cgltf_accessor const*
findAttribute(cgltf_primitive const* p_primitive,
              cgltf_attribute_type p_type,
              cgltf_int p_index = 0)
{
    for (std::size_t i = 0u; i < p_primitive->attributes_count; ++i)
    {
        if ((p_primitive->attributes[i].type == p_type) &&
            (p_primitive->attributes[i].index == p_index))
        {
            return p_primitive->attributes[i].data;
        }
    }
    return nullptr;
}

[[nodiscard]] gloop::Result<MeshAsset>
meshFromPrimitive(cgltf_primitive const* p_primitive)
{
    cgltf_accessor const* positions = findAttribute(p_primitive, cgltf_attribute_type_position);
    if (positions == nullptr)
    {
        return gloop::failure("glTF primitive has no POSITION attribute");
    }
    if (positions->count > 65535u)
    {
        return gloop::failure("glTF primitive exceeds the 65535 vertex limit of "
                            "the current MeshAsset");
    }

    cgltf_accessor const* normals =
        findAttribute(p_primitive, cgltf_attribute_type_normal);
    cgltf_accessor const* uvs =
        findAttribute(p_primitive, cgltf_attribute_type_texcoord);
    cgltf_accessor const* joints =
        findAttribute(p_primitive, cgltf_attribute_type_joints);
    cgltf_accessor const* weights =
        findAttribute(p_primitive, cgltf_attribute_type_weights);

    std::vector<float> position_data(positions->count * 3u);
    if (cgltf_accessor_unpack_floats(positions, position_data.data(),
                                     position_data.size()) == 0)
    {
        return gloop::failure("failed to read glTF POSITION data");
    }

    std::vector<float> normal_data;
    if (normals != nullptr)
    {
        normal_data.resize(normals->count * 3u);
        if (cgltf_accessor_unpack_floats(normals, normal_data.data(),
                                         normal_data.size()) == 0)
        {
            return gloop::failure("failed to read glTF NORMAL data");
        }
    }

    std::vector<float> uv_data;
    if (uvs != nullptr)
    {
        uv_data.resize(uvs->count * 2u);
        if (cgltf_accessor_unpack_floats(uvs, uv_data.data(), uv_data.size()) ==
            0)
        {
            return gloop::failure("failed to read glTF TEXCOORD data");
        }
    }

    std::vector<float> weight_data;
    if (weights != nullptr)
    {
        weight_data.resize(weights->count * 4u);
        if (cgltf_accessor_unpack_floats(weights, weight_data.data(),
                                         weight_data.size()) == 0)
        {
            return gloop::failure("failed to read glTF WEIGHTS data");
        }
    }

    const bool skinned = (joints != nullptr) && (weights != nullptr);

    std::vector<MeshVertex> vertices;
    vertices.reserve(positions->count);
    AABB bounds;
    for (std::size_t i = 0u; i < positions->count; ++i)
    {
        MeshVertex vertex;
        vertex.position = Vector3f(position_data[(i * 3u) + 0u],
                                   position_data[(i * 3u) + 1u],
                                   position_data[(i * 3u) + 2u]);
        if (!normal_data.empty())
        {
            vertex.normal =
                Vector3f(normal_data[(i * 3u) + 0u],
                         normal_data[(i * 3u) + 1u],
                         normal_data[(i * 3u) + 2u]);
        }
        else
        {
            vertex.normal = vector::normalize(vertex.position);
        }
        if (!uv_data.empty())
        {
            vertex.uv = Vector2f(uv_data[(i * 2u) + 0u], uv_data[(i * 2u) + 1u]);
        }
        bounds.expand(vertex.position);
        vertices.push_back(vertex);
    }

    std::vector<std::uint16_t> joint_indices;
    std::vector<Vector4f> joint_weights;
    if (skinned)
    {
        joint_indices.resize(vertices.size() * 4u);
        joint_weights.resize(vertices.size());
        for (std::size_t i = 0u; i < vertices.size(); ++i)
        {
            cgltf_uint raw[4] = { 0u, 0u, 0u, 0u };
            if (cgltf_accessor_read_uint(joints, i, raw, 4u) == 0)
            {
                return gloop::failure("failed to read glTF JOINTS data");
            }
            joint_indices[(i * 4u) + 0u] = static_cast<std::uint16_t>(raw[0]);
            joint_indices[(i * 4u) + 1u] = static_cast<std::uint16_t>(raw[1]);
            joint_indices[(i * 4u) + 2u] = static_cast<std::uint16_t>(raw[2]);
            joint_indices[(i * 4u) + 3u] = static_cast<std::uint16_t>(raw[3]);
            joint_weights[i] = Vector4f(weight_data[(i * 4u) + 0u],
                                        weight_data[(i * 4u) + 1u],
                                        weight_data[(i * 4u) + 2u],
                                        weight_data[(i * 4u) + 3u]);
        }
    }

    std::vector<std::uint16_t> indices;
    if (p_primitive->indices != nullptr)
    {
        indices.resize(p_primitive->indices->count);
        for (std::size_t i = 0u; i < p_primitive->indices->count; ++i)
        {
            indices[i] = static_cast<std::uint16_t>(
                cgltf_accessor_read_index(p_primitive->indices, i));
        }
    }
    else
    {
        indices.reserve(vertices.size());
        for (std::uint16_t i = 0u; i < vertices.size(); ++i)
        {
            indices.push_back(i);
        }
    }

    GPU_TRY_ASSIGN(vertices_buffer,
                   gpu::Buffer<MeshVertex>::from(
                       std::span<const MeshVertex>(vertices),
                       gpu::BufferKind::Vertex,
                       skinned ? gpu::BufferUsage::Dynamic
                               : gpu::BufferUsage::Immutable));
    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));

    MeshAsset mesh;
    mesh.vertices = std::move(vertices_buffer);
    mesh.indices = std::move(index_buffer);
    mesh.index_count = indices.size();
    mesh.index_type = gpu::IndexType::UInt16;
    mesh.local_bounds = bounds;
    if (skinned)
    {
        mesh.rest_pose = std::move(vertices);
        mesh.joint_indices = std::move(joint_indices);
        mesh.joint_weights = std::move(joint_weights);
    }
    return mesh;
}

void applyNodeTransform(cgltf_node const* p_node, world::LocalTransform& p_local)
{
    if (p_node->has_matrix)
    {
        // glTF matrices are column-major, column-vector: translation sits at
        // indices 12, 13, 14 and the first three columns are the scaled basis.
        // Quatf::fromMatrix() reads a mathematical (column-vector) rotation.
        p_local.position = Vector3f(p_node->matrix[12],
                                    p_node->matrix[13],
                                    p_node->matrix[14]);

        const Vector3f axis_x(p_node->matrix[0], p_node->matrix[1],
                              p_node->matrix[2]);
        const Vector3f axis_y(p_node->matrix[4], p_node->matrix[5],
                              p_node->matrix[6]);
        const Vector3f axis_z(p_node->matrix[8], p_node->matrix[9],
                              p_node->matrix[10]);
        const float scale_x = vector::norm(axis_x);
        const float scale_y = vector::norm(axis_y);
        const float scale_z = vector::norm(axis_z);
        p_local.scale = Vector3f(scale_x, scale_y, scale_z);

        Matrix44f rotation_matrix(matrix::Identity);
        if (scale_x > 1.0e-8f)
        {
            const Vector3f n = axis_x / scale_x;
            rotation_matrix(0, 0) = n.x;
            rotation_matrix(1, 0) = n.y;
            rotation_matrix(2, 0) = n.z;
        }
        if (scale_y > 1.0e-8f)
        {
            const Vector3f n = axis_y / scale_y;
            rotation_matrix(0, 1) = n.x;
            rotation_matrix(1, 1) = n.y;
            rotation_matrix(2, 1) = n.z;
        }
        if (scale_z > 1.0e-8f)
        {
            const Vector3f n = axis_z / scale_z;
            rotation_matrix(0, 2) = n.x;
            rotation_matrix(1, 2) = n.y;
            rotation_matrix(2, 2) = n.z;
        }
        p_local.rotation = Quatf::fromMatrix(rotation_matrix);
        return;
    }

    if (p_node->has_translation)
    {
        p_local.position = Vector3f(p_node->translation[0],
                                      p_node->translation[1],
                                      p_node->translation[2]);
    }
    if (p_node->has_rotation)
    {
        p_local.rotation = Quatf(p_node->rotation[3], p_node->rotation[0],
                                 p_node->rotation[1], p_node->rotation[2]);
    }
    if (p_node->has_scale)
    {
        p_local.scale = Vector3f(p_node->scale[0], p_node->scale[1],
                                 p_node->scale[2]);
    }
}

[[nodiscard]] gloop::Result<MaterialInstanceId> materialInstanceFromGltf(
    cgltf_material const* p_material,
    MaterialId p_shared_material,
    AssetManager& p_assets,
    TextureMap& p_textures,
    std::size_t& p_texture_count)
{
    MaterialInstance instance;
    instance.material = p_shared_material;
    if ((p_material != nullptr) && p_material->has_pbr_metallic_roughness)
    {
        const cgltf_pbr_metallic_roughness& pbr =
            p_material->pbr_metallic_roughness;
        instance.base_color_factor =
            Vector3f(pbr.base_color_factor[0], pbr.base_color_factor[1],
                     pbr.base_color_factor[2]);
        if (pbr.base_color_texture.texture != nullptr)
        {
            GPU_TRY_ASSIGN(texture_id,
                           importImage(pbr.base_color_texture.texture->image,
                                       p_assets,
                                       p_textures,
                                       p_texture_count));
            instance.base_color_texture = texture_id;
        }
    }

    const std::string name =
        (p_material != nullptr && p_material->name != nullptr)
            ? p_material->name
            : "gltf-material";
    GPU_TRY_ASSIGN(id, p_assets.addMaterialInstance(name, instance));
    return id;
}

[[nodiscard]] std::size_t nodeIndex(cgltf_data const* p_data,
                                    cgltf_node const* p_node)
{
    return static_cast<std::size_t>(p_node - p_data->nodes);
}

[[nodiscard]] gloop::Status importNode(cgltf_node* p_node,
                                     cgltf_data* p_data,
                                     AssetManager& p_assets,
                                     world::World& p_world,
                                     world::Entity p_parent,
                                     MaterialId p_shared_material,
                                     TextureMap& p_textures,
                                     std::vector<world::Entity>& p_nodes,
                                     GltfImport& p_result)
{
    const std::string node_name =
        (p_node->name != nullptr) ? p_node->name : "gltf-node";
    world::Entity entity = p_world.create(node_name);
    p_nodes[nodeIndex(p_data, p_node)] = entity;
    ++p_result.node_count;
    if (p_parent.valid())
    {
        GPU_TRY(p_world.setParent(entity, p_parent));
    }

    applyNodeTransform(p_node, p_world.transform(entity));

    if (p_node->mesh != nullptr)
    {
        for (std::size_t i = 0u; i < p_node->mesh->primitives_count; ++i)
        {
            GPU_TRY_ASSIGN(mesh,
                           meshFromPrimitive(&p_node->mesh->primitives[i]));
            GPU_TRY_ASSIGN(mesh_id,
                           p_assets.addMesh(node_name + "-mesh-" + std::to_string(i),
                                            std::move(mesh)));
            ++p_result.mesh_count;

            GPU_TRY_ASSIGN(instance_id,
                           materialInstanceFromGltf(
                               p_node->mesh->primitives[i].material,
                               p_shared_material,
                               p_assets,
                               p_textures,
                               p_result.texture_count));

            world::MeshRenderer renderer;
            renderer.mesh = mesh_id;
            renderer.material_instance = instance_id;
            p_world.add(entity, renderer);
            break;
        }
    }

    for (std::size_t i = 0u; i < p_node->children_count; ++i)
    {
        GPU_TRY(importNode(p_node->children[i],
                           p_data,
                           p_assets,
                           p_world,
                           entity,
                           p_shared_material,
                           p_textures,
                           p_nodes,
                           p_result));
    }
    return gloop::success();
}

[[nodiscard]] gloop::Result<SkinAssetId>
importSkinAsset(cgltf_skin* p_skin,
                AssetManager& p_assets,
                std::size_t p_index)
{
    SkinAsset asset;
    if (p_skin->inverse_bind_matrices != nullptr)
    {
        asset.inverse_bind.resize(p_skin->joints_count);
        std::vector<float> raw(p_skin->joints_count * 16u);
        if (cgltf_accessor_unpack_floats(p_skin->inverse_bind_matrices,
                                         raw.data(),
                                         raw.size()) == 0)
        {
            return gloop::failure("failed to read glTF inverse bind matrices");
        }
        for (std::size_t i = 0u; i < p_skin->joints_count; ++i)
        {
            // glTF is column-major. This library stores the same 16 floats so
            // a shader upload with GL_FALSE sees the original columns.
            std::memcpy(asset.inverse_bind[i].data(),
                        raw.data() + (i * 16u),
                        sizeof(float) * 16u);
        }
    }
    else
    {
        asset.inverse_bind.assign(p_skin->joints_count,
                                  Matrix44f(matrix::Identity));
    }
    const std::string name = (p_skin->name != nullptr)
                                 ? p_skin->name
                                 : ("gltf-skin-" + std::to_string(p_index));
    GPU_TRY_ASSIGN(id, p_assets.addSkin(name, std::move(asset)));
    return id;
}

[[nodiscard]] gloop::Status bindSkins(cgltf_data* p_data,
                                    AssetManager& p_assets,
                                    world::World& p_world,
                                    std::vector<world::Entity> const& p_nodes,
                                    GltfImport& p_result)
{
    std::unordered_map<cgltf_skin*, SkinAssetId> skins;
    for (std::size_t i = 0u; i < p_data->nodes_count; ++i)
    {
        cgltf_node* node = &p_data->nodes[i];
        if ((node->skin == nullptr) || (node->mesh == nullptr))
        {
            continue;
        }
        const world::Entity entity = p_nodes[i];
        if (!entity.valid() || !p_world.has<world::MeshRenderer>(entity))
        {
            continue;
        }
        SkinAssetId skin_id;
        const auto cached = skins.find(node->skin);
        if (cached != skins.end())
        {
            skin_id = cached->second;
        }
        else
        {
            const std::size_t skin_index =
                static_cast<std::size_t>(node->skin - p_data->skins);
            GPU_TRY_ASSIGN(created,
                           importSkinAsset(node->skin, p_assets, skin_index));
            skin_id = created;
            skins.emplace(node->skin, skin_id);
            ++p_result.skin_count;
        }

        world::MeshRenderer const& renderer =
            p_world.get<world::MeshRenderer>(entity);
        assets::MeshAsset* mesh = p_assets.mesh(renderer.mesh);
        if (mesh != nullptr)
        {
            mesh->skin = skin_id;
        }

        world::SkinInstance instance;
        instance.joints.resize(node->skin->joints_count);
        for (std::size_t j = 0u; j < node->skin->joints_count; ++j)
        {
            if (node->skin->joints[j] != nullptr)
            {
                instance.joints[j] =
                    p_nodes[nodeIndex(p_data, node->skin->joints[j])];
            }
        }
        p_world.add(entity, std::move(instance));
    }
    return gloop::success();
}

[[nodiscard]] gloop::Status importAnimations(cgltf_data* p_data,
                                           AssetManager& p_assets,
                                           std::vector<world::Entity> const& p_nodes,
                                           std::string const& p_path,
                                           GltfImport& p_result)
{
    for (std::size_t i = 0u; i < p_data->animations_count; ++i)
    {
        cgltf_animation const& source = p_data->animations[i];
        AnimationClip clip;
        clip.name = (source.name != nullptr) ? source.name
                                             : ("clip-" + std::to_string(i));
        clip.channels.reserve(source.channels_count);
        for (std::size_t c = 0u; c < source.channels_count; ++c)
        {
            cgltf_animation_channel const& channel = source.channels[c];
            if ((channel.target_node == nullptr) ||
                (channel.sampler == nullptr) ||
                (channel.sampler->input == nullptr) ||
                (channel.sampler->output == nullptr))
            {
                continue;
            }
            if ((channel.target_path != cgltf_animation_path_type_translation) &&
                (channel.target_path != cgltf_animation_path_type_rotation) &&
                (channel.target_path != cgltf_animation_path_type_scale))
            {
                continue;
            }

            AnimationChannel curve;
            curve.target = p_nodes[nodeIndex(p_data, channel.target_node)];
            if (!curve.target.valid())
            {
                continue;
            }
            if (channel.target_path == cgltf_animation_path_type_translation)
            {
                curve.path = AnimationPath::Translation;
            }
            else if (channel.target_path == cgltf_animation_path_type_rotation)
            {
                curve.path = AnimationPath::Rotation;
            }
            else
            {
                curve.path = AnimationPath::Scale;
            }
            curve.interpolation =
                (channel.sampler->interpolation == cgltf_interpolation_type_step)
                    ? AnimationInterpolation::Step
                    : AnimationInterpolation::Linear;

            curve.times.resize(channel.sampler->input->count);
            if (cgltf_accessor_unpack_floats(channel.sampler->input,
                                             curve.times.data(),
                                             curve.times.size()) == 0)
            {
                return gloop::failure("failed to read glTF animation times");
            }
            const std::size_t components =
                (curve.path == AnimationPath::Rotation) ? 4u : 3u;
            curve.values.resize(channel.sampler->output->count * components);
            if (cgltf_accessor_unpack_floats(channel.sampler->output,
                                             curve.values.data(),
                                             curve.values.size()) == 0)
            {
                return gloop::failure("failed to read glTF animation values");
            }
            if (!curve.times.empty())
            {
                clip.duration = std::max(clip.duration, curve.times.back());
            }
            clip.channels.push_back(std::move(curve));
        }
        const std::string asset_name = p_path + "#" + clip.name;
        GPU_TRY_ASSIGN(id, p_assets.addAnimation(asset_name, std::move(clip)));
        p_result.animations.push_back(id);
    }
    return gloop::success();
}

} // namespace

//------------------------------------------------------------------------------
gloop::Result<GltfImport> importGltf(std::string const& p_path,
                                   AssetManager& p_assets,
                                   world::World& p_world,
                                   world::Entity p_parent,
                                   MaterialId p_shared_material)
{
    cgltf_options options{};
    cgltf_data* data = nullptr;
    cgltf_result parse_result =
        cgltf_parse_file(&options, p_path.c_str(), &data);
    if (parse_result != cgltf_result_success)
    {
        return gloop::failure("cgltf_parse_file failed for '" + p_path + "'");
    }

    if (cgltf_load_buffers(&options, data, p_path.c_str()) != cgltf_result_success)
    {
        cgltf_free(data);
        return gloop::failure("cgltf_load_buffers failed for '" + p_path + "'");
    }

    MaterialId shared_material = p_shared_material;
    if (!shared_material.valid())
    {
        GPU_TRY_ASSIGN(material, makePbrMaterial());
        // GPU_TRY_ASSIGN always declares a new name; assigning back into the
        // outer id is required or every instance points at an empty Material.
        GPU_TRY_ASSIGN(created,
                       p_assets.addMaterial("gltf-pbr", std::move(material)));
        shared_material = created;
    }

    GltfImport result;
    TextureMap textures;

    if ((data->scenes_count == 0u) || (data->scene == nullptr))
    {
        cgltf_free(data);
        return gloop::failure("glTF file has no default scene");
    }

    // Identity wrapper so the caller can frame the model without overwriting
    // the file's node scale (Duck.glb stores centimetres and a 0.01 root).
    result.root = p_world.create("gltf-root");
    if (p_parent.valid())
    {
        GPU_TRY(p_world.setParent(result.root, p_parent));
    }

    std::vector<world::Entity> nodes(data->nodes_count);
    cgltf_scene* scene = data->scene;
    for (std::size_t i = 0u; i < scene->nodes_count; ++i)
    {
        GPU_TRY(importNode(scene->nodes[i],
                           data,
                           p_assets,
                           p_world,
                           result.root,
                           shared_material,
                           textures,
                           nodes,
                           result));
    }

    GPU_TRY(bindSkins(data, p_assets, p_world, nodes, result));
    GPU_TRY(importAnimations(data, p_assets, nodes, p_path, result));
    if (!result.animations.empty())
    {
        world::Animator animator;
        animator.clip = result.animations.front();
        for (AnimationClipId const id : result.animations)
        {
            AnimationClip const* clip = p_assets.animation(id);
            if ((clip != nullptr) && (clip->name == "Walk"))
            {
                animator.clip = id;
                break;
            }
        }
        p_world.add(result.root, animator);
        result.animator = result.root;
    }

    cgltf_free(data);
    p_world.update();
    return result;
}

} // namespace assets

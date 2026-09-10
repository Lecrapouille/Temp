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

#include "World/AnimationSystem.hpp"

#include "Assets/AssetManager.hpp"
#include "Math/Transformation.hpp"
#include "World/Components/Animator.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/SkinInstance.hpp"
#include "World/World.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace world
{

namespace
{

[[nodiscard]] Quatf slerp(Quatf p_a, Quatf p_b, float p_t)
{
    float dot = (p_a.a * p_b.a) + (p_a.b * p_b.b) + (p_a.c * p_b.c) +
                (p_a.d * p_b.d);
    if (dot < 0.0f)
    {
        p_b = Quatf(-p_b.a, -p_b.b, -p_b.c, -p_b.d);
        dot = -dot;
    }
    if (dot > 0.9995f)
    {
        Quatf result(p_a.a + (p_t * (p_b.a - p_a.a)),
                     p_a.b + (p_t * (p_b.b - p_a.b)),
                     p_a.c + (p_t * (p_b.c - p_a.c)),
                     p_a.d + (p_t * (p_b.d - p_a.d)));
        result.normalize();
        return result;
    }
    const float theta = std::acos(std::min(dot, 1.0f));
    const float s = std::sin(theta);
    const float w0 = std::sin((1.0f - p_t) * theta) / s;
    const float w1 = std::sin(p_t * theta) / s;
    return Quatf((w0 * p_a.a) + (w1 * p_b.a), (w0 * p_a.b) + (w1 * p_b.b),
                 (w0 * p_a.c) + (w1 * p_b.c), (w0 * p_a.d) + (w1 * p_b.d));
}

void locateKey(std::vector<float> const& p_times,
               float p_time,
               std::size_t& p_left,
               std::size_t& p_right,
               float& p_blend)
{
    p_left = 0u;
    p_right = 0u;
    p_blend = 0.0f;
    if (p_times.empty())
    {
        return;
    }
    if ((p_times.size() == 1u) || (p_time <= p_times.front()))
    {
        return;
    }
    if (p_time >= p_times.back())
    {
        p_left = p_times.size() - 1u;
        p_right = p_left;
        return;
    }
    const auto it =
        std::upper_bound(p_times.begin(), p_times.end(), p_time);
    p_right = static_cast<std::size_t>(it - p_times.begin());
    p_left = p_right - 1u;
    const float span = p_times[p_right] - p_times[p_left];
    p_blend = (span > 1.0e-8f) ? ((p_time - p_times[p_left]) / span) : 0.0f;
}

void applyChannel(assets::AnimationChannel const& p_channel,
                  float p_time,
                  LocalTransform& p_local)
{
    if (p_channel.times.empty() || p_channel.values.empty())
    {
        return;
    }

    std::size_t left = 0u;
    std::size_t right = 0u;
    float blend = 0.0f;
    locateKey(p_channel.times, p_time, left, right, blend);
    if (p_channel.interpolation == assets::AnimationInterpolation::Step)
    {
        blend = 0.0f;
        right = left;
    }

    if (p_channel.path == assets::AnimationPath::Rotation)
    {
        const std::size_t i0 = left * 4u;
        const std::size_t i1 = right * 4u;
        if ((i0 + 3u >= p_channel.values.size()) ||
            (i1 + 3u >= p_channel.values.size()))
        {
            return;
        }
        // glTF stores xyzw; Quatf is wxyz.
        const Quatf a(p_channel.values[i0 + 3u], p_channel.values[i0 + 0u],
                      p_channel.values[i0 + 1u], p_channel.values[i0 + 2u]);
        const Quatf b(p_channel.values[i1 + 3u], p_channel.values[i1 + 0u],
                      p_channel.values[i1 + 1u], p_channel.values[i1 + 2u]);
        p_local.rotation = slerp(a, b, blend);
        return;
    }

    const std::size_t i0 = left * 3u;
    const std::size_t i1 = right * 3u;
    if ((i0 + 2u >= p_channel.values.size()) ||
        (i1 + 2u >= p_channel.values.size()))
    {
        return;
    }
    const Vector3f a(p_channel.values[i0 + 0u], p_channel.values[i0 + 1u],
                     p_channel.values[i0 + 2u]);
    const Vector3f b(p_channel.values[i1 + 0u], p_channel.values[i1 + 1u],
                     p_channel.values[i1 + 2u]);
    const Vector3f mixed = a + ((b - a) * blend);
    if (p_channel.path == assets::AnimationPath::Translation)
    {
        p_local.position = mixed;
    }
    else
    {
        p_local.scale = mixed;
    }
}

[[nodiscard]] Vector3f transformPoint(Matrix44f const& p_matrix,
                                      Vector3f const& p_point)
{
    const Vector4f homogeneous(p_point.x, p_point.y, p_point.z, 1.0f);
    const Vector4f out = homogeneous * p_matrix;
    return Vector3f(out.x, out.y, out.z);
}

[[nodiscard]] Vector3f transformVector(Matrix44f const& p_matrix,
                                       Vector3f const& p_vector)
{
    return Vector3f((p_vector.x * p_matrix(0, 0)) +
                        (p_vector.y * p_matrix(1, 0)) +
                        (p_vector.z * p_matrix(2, 0)),
                    (p_vector.x * p_matrix(0, 1)) +
                        (p_vector.y * p_matrix(1, 1)) +
                        (p_vector.z * p_matrix(2, 1)),
                    (p_vector.x * p_matrix(0, 2)) +
                        (p_vector.y * p_matrix(1, 2)) +
                        (p_vector.z * p_matrix(2, 2)));
}

} // namespace

//------------------------------------------------------------------------------
gloop::Status AnimationSystem::sample(World& p_world,
                                    assets::AssetManager const& p_assets,
                                    float p_dt)
{
    auto& store = p_world.components<Animator>();
    auto const animators = store.components();
    for (std::size_t i = 0u; i < animators.size(); ++i)
    {
        Animator& animator = animators[i];
        if (!animator.playing || !animator.clip.valid())
        {
            continue;
        }
        assets::AnimationClip const* clip = p_assets.animation(animator.clip);
        if (clip == nullptr)
        {
            continue;
        }
        animator.time += p_dt * animator.speed;
        if (clip->duration > 0.0f)
        {
            if (animator.loop)
            {
                animator.time = std::fmod(animator.time, clip->duration);
                if (animator.time < 0.0f)
                {
                    animator.time += clip->duration;
                }
            }
            else if (animator.time > clip->duration)
            {
                animator.time = clip->duration;
                animator.playing = false;
            }
        }
        for (assets::AnimationChannel const& channel : clip->channels)
        {
            if (!p_world.alive(channel.target))
            {
                continue;
            }
            applyChannel(channel, animator.time, p_world.transform(channel.target));
        }
    }
    return gloop::success();
}

//------------------------------------------------------------------------------
gloop::Status AnimationSystem::skin(World& p_world,
                                  assets::AssetManager& p_assets)
{
    auto const& skins = p_world.components<SkinInstance>();
    auto const instances = skins.components();
    auto const entities = skins.entities();
    std::vector<Matrix44f> joint_matrices;
    std::vector<assets::MeshVertex> posed;

    for (std::size_t i = 0u; i < instances.size(); ++i)
    {
        world::MeshRenderer const* renderer =
            p_world.tryGet<world::MeshRenderer>(entities[i]);
        if (renderer == nullptr)
        {
            continue;
        }
        assets::MeshAsset* mesh = p_assets.mesh(renderer->mesh);
        if ((mesh == nullptr) || mesh->rest_pose.empty() ||
            !mesh->skin.valid())
        {
            continue;
        }
        assets::SkinAsset const* skin_asset = p_assets.skin(mesh->skin);
        if (skin_asset == nullptr)
        {
            continue;
        }

        const Matrix44f mesh_world = p_world.worldMatrix(entities[i]);
        const Matrix44f inverse_mesh = matrix::inverse(mesh_world);
        const std::size_t joint_count = skin_asset->inverse_bind.size();
        joint_matrices.assign(joint_count, Matrix44f(matrix::Identity));
        for (std::size_t j = 0u; j < joint_count; ++j)
        {
            if (j >= instances[i].joints.size() ||
                !p_world.alive(instances[i].joints[j]))
            {
                continue;
            }
            // Library matrices are stored transposed for OpenGL, so the
            // column-vector product inv(mesh) * joint * ibm becomes this
            // multiply order, the same as world = local * parent.
            joint_matrices[j] = skin_asset->inverse_bind[j] *
                                p_world.worldMatrix(instances[i].joints[j]) *
                                inverse_mesh;
        }

        posed = mesh->rest_pose;
        AABB bounds;
        for (std::size_t v = 0u; v < posed.size(); ++v)
        {
            const Vector4f weights = (v < mesh->joint_weights.size())
                                         ? mesh->joint_weights[v]
                                         : Vector4f(1.0f, 0.0f, 0.0f, 0.0f);
            Vector3f position(0.0f, 0.0f, 0.0f);
            Vector3f normal(0.0f, 0.0f, 0.0f);
            float weight_sum = 0.0f;
            for (std::size_t k = 0u; k < 4u; ++k)
            {
                const float weight = weights[k];
                if (weight <= 0.0f)
                {
                    continue;
                }
                const std::size_t joint =
                    (v * 4u + k) < mesh->joint_indices.size()
                        ? static_cast<std::size_t>(
                              mesh->joint_indices[(v * 4u) + k])
                        : 0u;
                if (joint >= joint_matrices.size())
                {
                    continue;
                }
                Matrix44f const& skin_matrix = joint_matrices[joint];
                position +=
                    transformPoint(skin_matrix, mesh->rest_pose[v].position) *
                    weight;
                normal +=
                    transformVector(skin_matrix, mesh->rest_pose[v].normal) *
                    weight;
                weight_sum += weight;
            }
            if (weight_sum <= 0.0f)
            {
                position = mesh->rest_pose[v].position;
                normal = mesh->rest_pose[v].normal;
            }
            posed[v].position = position;
            const float nlen = vector::norm(normal);
            posed[v].normal =
                (nlen > 1.0e-8f) ? (normal / nlen) : mesh->rest_pose[v].normal;
            bounds.expand(posed[v].position);
        }
        GPU_TRY(mesh->vertices.write(std::span<const assets::MeshVertex>(posed)));
        if (!bounds.empty())
        {
            mesh->local_bounds = bounds;
        }
    }
    return gloop::success();
}

//------------------------------------------------------------------------------
gloop::Status AnimationSystem::tick(World& p_world,
                                  assets::AssetManager& p_assets,
                                  float p_dt)
{
    GPU_TRY(sample(p_world, p_assets, p_dt));
    p_world.update();
    return skin(p_world, p_assets);
}

} // namespace world

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

#include "04_World/26_Skybox.hpp"

#include "Assets/Primitives.hpp"
#include "Common/DataPath.hpp"
#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <array>
#include <cmath>
#include <string>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr char const* SKY_VERTEX = R"(#version 450 core
in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vDirection;

void main()
{
    vec4 world = model * vec4(position, 1.0);
    vDirection = world.xyz;
    vec4 clip = projection * view * world;
    gl_Position = clip.xyww;
}
)";

constexpr char const* SKY_FRAGMENT = R"(#version 450 core
in vec3 vDirection;

uniform samplerCube skybox;

out vec4 oColor;

void main()
{
    oColor = texture(skybox, normalize(vDirection));
}
)";

[[nodiscard]] Matrix44f viewWithoutTranslation(Matrix44f const& p_view)
{
    Matrix44f view = p_view;
    view[3] = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    return view;
}

} // namespace

//------------------------------------------------------------------------------
std::string Skybox::description() const
{
    return "Six JPG faces from OpenGLCppWrapper-data become a cube map drawn "
           "from the inside, then a lit cube spins at the centre. Inspired by "
           "the legacy 09_SkyBoxTextureCube example and learnopengl cubemaps.";
}

//------------------------------------------------------------------------------
gpu::Status Skybox::setUp()
{
    const std::array<std::pair<char const*, char const*>, 6u> faces{ {
        { "right.jpg", "+X" },
        { "left.jpg", "-X" },
        { "top.jpg", "+Y" },
        { "bottom.jpg", "-Y" },
        { "front.jpg", "+Z" },
        { "back.jpg", "-Z" },
    } };

    std::array<std::string, 6u> paths;
    for (std::size_t i = 0u; i < faces.size(); ++i)
    {
        paths[i] = dataPath(faces[i].first);
        if (paths[i].empty())
        {
            return gpu::failure(
                "26_Skybox needs the six skybox JPG faces in "
                "external/OpenGLCppWrapper-data/ (right, left, top, bottom, "
                "front, back). Run make download in external/.");
        }
    }

    GPU_TRY_ASSIGN(cubemap, gpu::Texture::cubeFromFiles(paths));
    m_sky_texture = std::move(cubemap);

    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(SKY_VERTEX, SKY_FRAGMENT));
    m_sky_program = std::move(program);

    const gpu::VertexLayout layout = GPU_LAYOUT(assets::MeshVertex, position);
    gpu::RenderState state;
    state.depth_test = true;
    state.depth_write = false;
    state.depth_func = gpu::CompareFunc::LessEqual;
    state.cull = gpu::CullMode::Front;
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<assets::MeshVertex>(m_sky_program,
                                                             layout,
                                                             state));
    m_sky_pipeline = std::move(pipeline);

    GPU_TRY_ASSIGN(room_mesh, assets::makeCube());
    GPU_TRY_ASSIGN(room_id, m_assets.addMesh("skybox-room", std::move(room_mesh)));
    m_room_mesh = room_id;

    GPU_TRY_ASSIGN(cube_mesh, assets::makeCube());
    GPU_TRY_ASSIGN(cube_id, m_assets.addMesh("cube", std::move(cube_mesh)));
    m_cube_mesh = cube_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("lit", std::move(lit)));
    GPU_TRY_ASSIGN(
        cube_mat,
        m_assets.addMaterialInstance(
            "cube",
            assets::MaterialInstance{ material_id,
                                      Vector3f(0.85f, 0.55f, 0.25f) }));
    m_cube_material = cube_mat;

    m_cube = m_world.create("Cube");
    m_world.transform(m_cube).scale = Vector3f(1.2f);
    m_world.add(m_cube, world::MeshRenderer{ m_cube_mesh, m_cube_material });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 60.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 200.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 0.0f, 4.5f);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.18f, 0.18f, 0.20f);
    m_scene.environment().default_light_direction =
        Vector3f(0.2f, -0.9f, -0.3f);

    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status Skybox::drawSkybox(render::RenderSnapshot const& p_snapshot)
{
    GPU_TRY(m_sky_texture.bind(0u));
    GPU_TRY(m_sky_program.set("skybox", 0));
    GPU_TRY(m_sky_program.set("view",
                              viewWithoutTranslation(p_snapshot.camera.view)));
    GPU_TRY(m_sky_program.set("projection", p_snapshot.camera.projection));

    assets::MeshAsset const* room = m_assets.mesh(m_room_mesh);
    if (room == nullptr)
    {
        return gpu::failure("skybox room mesh is missing from the AssetManager");
    }

    Matrix44f model(matrix::Type::Identity);
    model[0].x = 50.0f;
    model[1].y = 50.0f;
    model[2].z = 50.0f;
    GPU_TRY(m_sky_program.set("model", model));
    return gpu::drawIndexed(m_sky_pipeline,
                            room->vertices.handle(),
                            room->indices.handle(),
                            room->index_type,
                            room->index_count);
}

//------------------------------------------------------------------------------
gpu::Status Skybox::draw(Frame const& p_frame)
{
    const float t = p_frame.total;
    m_world.transform(m_camera).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(t * 0.15f),
                             Vector3f(0.0f, 1.0f, 0.0f)) *
        Quatf::fromAngleAxis(units::angle::radian_t(-0.35f),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_cube).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(t * 0.9f),
                             Vector3f(0.0f, 1.0f, 0.0f)) *
        Quatf::fromAngleAxis(units::angle::radian_t(t * 0.5f),
                             Vector3f(1.0f, 0.0f, 0.0f));

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

    GPU_TRY(drawSkybox(snapshot));
    return m_renderer.render(pass, snapshot, m_assets);
}

} // namespace examples

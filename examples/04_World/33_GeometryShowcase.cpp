#include "04_World/33_GeometryShowcase.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

namespace
{

Quatf tiltZShapeUp()
{
    return Quatf::fromAngleAxis(units::angle::degree_t(90.0),
                                Vector3f(1.0f, 0.0f, 0.0f));
}

Quatf layFlatOnGround()
{
    return Quatf::fromAngleAxis(units::angle::degree_t(-90.0),
                                Vector3f(1.0f, 0.0f, 0.0f));
}

gpu::Status addMaterialInstance(assets::AssetManager& p_assets,
                                char const* p_name,
                                assets::MaterialId p_material,
                                Vector3f p_color,
                                assets::MaterialInstanceId& p_out)
{
    assets::MaterialInstance instance;
    instance.material = p_material;
    instance.color = p_color;
    GPU_TRY_ASSIGN(id, p_assets.addMaterialInstance(p_name, instance));
    p_out = id;
    return gpu::success();
}

} // namespace

std::string GeometryShowcase::description() const
{
    return "Legacy geometry and materials on a shared plane: cube, box, "
           "sphere, cone, cylinder, pyramid and tube, each with lit, PBR, "
           "depth or normals shading. The camera orbits the plateau.";
}

gpu::Status GeometryShowcase::addProp(char const* p_mesh_name,
                                      assets::MeshAsset p_mesh,
                                      assets::MaterialInstanceId p_material,
                                      Vector3f p_position,
                                      Quatf p_rotation,
                                      Vector3f p_scale)
{
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh(p_mesh_name, std::move(p_mesh)));
    world::Entity entity = m_world.create();
    world::LocalTransform& local = m_world.transform(entity);
    local.position = p_position;
    local.rotation = p_rotation;
    local.scale = p_scale;
    m_world.add(entity, world::MeshRenderer{ mesh_id, p_material });
    m_props.push_back(entity);
    return gpu::success();
}

gpu::Status GeometryShowcase::setUp()
{
    GPU_TRY_ASSIGN(lit_mat, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(lit_id, m_assets.addMaterial("lit", std::move(lit_mat)));
    m_lit_material = lit_id;

    GPU_TRY_ASSIGN(pbr_mat, assets::makePbrMaterial());
    GPU_TRY_ASSIGN(pbr_id, m_assets.addMaterial("pbr", std::move(pbr_mat)));
    m_pbr_material = pbr_id;

    GPU_TRY_ASSIGN(depth_mat, assets::makeDepthMaterial());
    GPU_TRY_ASSIGN(depth_id, m_assets.addMaterial("depth", std::move(depth_mat)));
    m_depth_material = depth_id;

    GPU_TRY_ASSIGN(normals_mat, assets::makeNormalsMaterial());
    GPU_TRY_ASSIGN(normals_id,
                   m_assets.addMaterial("normals", std::move(normals_mat)));
    m_normals_material = normals_id;

    assets::MaterialInstanceId platform_mat;
    GPU_TRY(addMaterialInstance(m_assets, "platform", m_lit_material,
                                Vector3f(0.42f, 0.44f, 0.48f), platform_mat));

    assets::MaterialInstanceId lit_red;
    assets::MaterialInstanceId lit_green;
    assets::MaterialInstanceId lit_orange;
    assets::MaterialInstanceId lit_cyan;
    assets::MaterialInstanceId lit_yellow;
    assets::MaterialInstanceId pbr_blue;
    assets::MaterialInstanceId depth_inst;
    assets::MaterialInstanceId normals_inst;

    GPU_TRY(addMaterialInstance(m_assets, "lit_red", m_lit_material,
                                Vector3f(0.85f, 0.28f, 0.24f), lit_red));
    GPU_TRY(addMaterialInstance(m_assets, "lit_green", m_lit_material,
                                Vector3f(0.30f, 0.72f, 0.38f), lit_green));
    GPU_TRY(addMaterialInstance(m_assets, "lit_orange", m_lit_material,
                                Vector3f(0.92f, 0.55f, 0.18f), lit_orange));
    GPU_TRY(addMaterialInstance(m_assets, "lit_cyan", m_lit_material,
                                Vector3f(0.25f, 0.70f, 0.82f), lit_cyan));
    GPU_TRY(addMaterialInstance(m_assets, "lit_yellow", m_lit_material,
                                Vector3f(0.90f, 0.82f, 0.25f), lit_yellow));
    {
        assets::MaterialInstance pbr;
        pbr.material = m_pbr_material;
        pbr.base_color_factor = Vector3f(0.28f, 0.45f, 0.92f);
        GPU_TRY_ASSIGN(id, m_assets.addMaterialInstance("pbr_blue", pbr));
        pbr_blue = id;
    }
    {
        assets::MaterialInstance depth;
        depth.material = m_depth_material;
        depth.depth_near = 8.0f;
        depth.depth_far = 22.0f;
        depth.opacity = 1.0f;
        GPU_TRY_ASSIGN(id, m_assets.addMaterialInstance("depth", depth));
        depth_inst = id;
    }
    {
        assets::MaterialInstance norm;
        norm.material = m_normals_material;
        norm.opacity = 1.0f;
        GPU_TRY_ASSIGN(id, m_assets.addMaterialInstance("normals", norm));
        normals_inst = id;
    }

    GPU_TRY_ASSIGN(plane, assets::makePlane(16.0f, 12.0f, 4u, 3u));
    GPU_TRY(addProp("platform", std::move(plane), platform_mat,
                    Vector3f(0.0f, 0.0f, 0.0f), layFlatOnGround()));

    constexpr float dx = 3.0f;
    constexpr float dz = 3.0f;
    const Quatf stand = tiltZShapeUp();

    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY(addProp("cube", std::move(cube), lit_red,
                    Vector3f(-dx, 0.5f, -dz)));

    GPU_TRY_ASSIGN(box, assets::makeBox(1.2f, 0.7f, 0.9f));
    GPU_TRY(addProp("box", std::move(box), lit_green,
                    Vector3f(0.0f, 0.35f, -dz)));

    GPU_TRY_ASSIGN(sphere, assets::makeSphere(0.45f, 20u, 24u));
    GPU_TRY(addProp("sphere", std::move(sphere), pbr_blue,
                    Vector3f(dx, 0.45f, -dz)));

    GPU_TRY_ASSIGN(cone, assets::makeCone(0.55f, 0.0f, 1.1f, 20u));
    GPU_TRY(addProp("cone", std::move(cone), lit_orange,
                    Vector3f(-dx, 0.55f, 0.0f), stand));

    GPU_TRY_ASSIGN(cylinder, assets::makeCylinder(0.42f, 1.0f, 24u));
    GPU_TRY(addProp("cylinder", std::move(cylinder), lit_cyan,
                    Vector3f(0.0f, 0.5f, 0.0f), stand));

    GPU_TRY_ASSIGN(pyramid, assets::makePyramid(0.65f, 1.0f));
    GPU_TRY(addProp("pyramid", std::move(pyramid), lit_yellow,
                    Vector3f(dx, 0.5f, 0.0f), stand));

    GPU_TRY_ASSIGN(tube, assets::makeTube(0.35f, 0.55f, 0.95f, 18u, false));
    GPU_TRY(addProp("tube", std::move(tube), normals_inst,
                    Vector3f(-dx, 0.48f, dz), stand));

    GPU_TRY_ASSIGN(depth_cube, assets::makeCube());
    GPU_TRY(addProp("depth_cube", std::move(depth_cube), depth_inst,
                    Vector3f(0.0f, 0.5f, dz)));

    GPU_TRY_ASSIGN(normal_sphere, assets::makeSphere(0.42f, 16u, 20u));
    GPU_TRY(addProp("normal_sphere", std::move(normal_sphere), normals_inst,
                    Vector3f(dx, 0.42f, dz)));

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.65),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.97f, 0.92f), 1.1f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 45.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 100.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 7.0f, 14.0f);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.06f, 0.07f, 0.10f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.22f, 0.22f, 0.24f);

    m_world.update();
    return gpu::success();
}

gpu::Status GeometryShowcase::draw(Frame const& p_frame)
{
    const float t = p_frame.total * 0.28f;
    const float radius = 14.0f;
    m_world.transform(m_camera).position =
        Vector3f(radius * std::sin(t), 6.5f, radius * std::cos(t));
    const Quatf yaw = Quatf::fromAngleAxis(units::angle::radian_t(t),
                                           Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.32f), Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_camera).rotation = yaw * pitch;

    const Quatf spin = Quatf::fromAngleAxis(
        units::angle::radian_t(p_frame.total * 0.45f),
        Vector3f(0.0f, 1.0f, 0.0f));
    for (std::size_t i = 1u; i < m_props.size(); ++i)
    {
        if (i >= 4u && i <= 7u)
        {
            m_world.transform(m_props[i]).rotation = spin * tiltZShapeUp();
        }
        else
        {
            m_world.transform(m_props[i]).rotation = spin;
        }
    }

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

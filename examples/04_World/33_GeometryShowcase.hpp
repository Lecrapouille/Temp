#pragma once

#include "Common/Example.hpp"

#include "Assets/AssetIds.hpp"
#include "Assets/AssetManager.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <vector>

namespace examples
{

class GeometryShowcase: public Example
{
public:

    GeometryShowcase() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "33_GeometryShowcase";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    [[nodiscard]] gpu::Status addProp(char const* p_mesh_name,
                                      assets::MeshAsset p_mesh,
                                      assets::MaterialInstanceId p_material,
                                      Vector3f p_position,
                                      Quatf p_rotation = Quatf{},
                                      Vector3f p_scale = Vector3f(1.0f, 1.0f, 1.0f));

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::MaterialId m_lit_material;
    assets::MaterialId m_pbr_material;
    assets::MaterialId m_depth_material;
    assets::MaterialId m_normals_material;

    world::Entity m_camera;
    world::Entity m_sun;
    std::vector<world::Entity> m_props;
};

} // namespace examples

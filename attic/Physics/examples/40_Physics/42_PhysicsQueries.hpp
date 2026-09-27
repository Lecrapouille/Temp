//=============================================================================
// Compages: A C++20 GPU, rendering and simulation library.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//=============================================================================

#pragma once

#include "Common/Example.hpp"

#include "Compages/Physics/PhysicsWorld.hpp"
#include "Compages/World/World.hpp"

namespace examples
{

class PhysicsQueries final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "42_PhysicsQueries";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    void draw(Frame const&) override;

private:

    world::World m_world;
    physics::PhysicsWorld m_physics;
    bool m_validated = false;
};

} // namespace examples

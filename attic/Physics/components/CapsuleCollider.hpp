//=============================================================================
// Compages: A C++20 GPU, rendering and simulation library.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//=============================================================================

#pragma once

namespace world
{

struct CapsuleCollider
{
    float radius = 0.5f;
    float height = 1.0f;
    bool trigger = false;
};

} // namespace world

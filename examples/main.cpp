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

#include "Common/Gallery.hpp"

#include "00_Basics/01_ClearScreen.hpp"
#include "00_Basics/02_Triangle.hpp"
#include "00_Basics/03_DynamicTriangle.hpp"
#include "00_Basics/04_TexturedQuad.hpp"
#include "00_Basics/05_IndexedCube.hpp"
#include "00_Basics/06_MultiPassMesh.hpp"
#include "00_Basics/07_RenderToTexture.hpp"
#include "00_Basics/08_Mandelbrot.hpp"
#include "00_Basics/28_MultiTextureBlend.hpp"
#include "00_Basics/29_PointSphere.hpp"
#include "00_Basics/31_ComplexShader.hpp"
#include "00_Basics/32_PostProcess.hpp"
#include "01_Scientific/09_HeightMap.hpp"
#include "01_Scientific/30_Terrain3D.hpp"
#include "01_Scientific/10_GameOfLife.hpp"
#include "01_Scientific/11_GrayScott.hpp"
#include "01_Scientific/12_Lorenz.hpp"
#include "02_Compute/13_ComputeParticles.hpp"
#include "02_Compute/14_Galaxy.hpp"
#include "03_Performance/15_SpriteBatch.hpp"
#include "03_Performance/16_IndirectDraw.hpp"
#include "04_World/17_MovingRobot.hpp"
#include "04_World/18_ManyCubes.hpp"
#include "04_World/19_SplitViews.hpp"
#include "04_World/20_CameraPick.hpp"
#include "04_World/21_GltfModel.hpp"
#include "04_World/22_TexturedSpheres.hpp"
#include "04_World/23_PrefabAndSave.hpp"
#include "04_World/24_MvpDemo.hpp"
#include "04_World/25_MiscLookAt.hpp"
#include "04_World/26_Skybox.hpp"
#include "04_World/27_TextureGallery.hpp"
#include "04_World/33_GeometryShowcase.hpp"
#include "04_World/34_AnimatedModel.hpp"
#include "04_World/35_PhysicsSandbox.hpp"
#include "04_World/36_GltfAnimation.hpp"

#include <iostream>

// ****************************************************************************
//! \file
//! \brief Every example, in one program.
//!
//! Run it with no argument to start at the first one, or with the name of an
//! example to start there:
//! \code
//! ./build/OpenGLCppWrapper-examples 05_IndexedCube
//! ./build/OpenGLCppWrapper-examples --check
//! \endcode
//!
//! With --check, every example is built, drawn for a few frames and closed, and
//! the program fails if any of them could not run or left anything behind on
//! the device. That is a leak test as much as a smoke test, and it is the
//! reason all of the examples are one program rather than seventeen.
//!
//! The examples are meant to be read in order. Each one adds a single idea to
//! the one before it, and says in its own comments which idea that is and why
//! it is there.
// ****************************************************************************

int main(int argc, char* argv[])
{
    examples::Gallery gallery;

    gallery.add<examples::ClearScreen>();
    gallery.add<examples::Triangle>();
    gallery.add<examples::DynamicTriangle>();
    gallery.add<examples::TexturedQuad>();
    gallery.add<examples::IndexedCube>();
    gallery.add<examples::MultiPassMesh>();
    gallery.add<examples::RenderToTexture>();
    gallery.add<examples::Mandelbrot>();
    gallery.add<examples::HeightMap>();
    gallery.add<examples::GameOfLife>();
    gallery.add<examples::GrayScott>();
    gallery.add<examples::Lorenz>();
    gallery.add<examples::ComputeParticles>();
    gallery.add<examples::Galaxy>();
    gallery.add<examples::SpriteBatch>();
    gallery.add<examples::IndirectDraw>();
    gallery.add<examples::MovingRobot>();
    gallery.add<examples::ManyCubes>();
    gallery.add<examples::SplitViews>();
    gallery.add<examples::CameraPick>();
    gallery.add<examples::GltfModel>();
    gallery.add<examples::TexturedSpheres>();
    gallery.add<examples::PrefabAndSave>();
    gallery.add<examples::MvpDemo>();
    gallery.add<examples::MiscLookAt>();
    gallery.add<examples::Skybox>();
    gallery.add<examples::TextureGallery>();
    gallery.add<examples::MultiTextureBlend>();
    gallery.add<examples::PointSphere>();
    gallery.add<examples::Terrain3D>();
    gallery.add<examples::ComplexShader>();
    gallery.add<examples::PostProcess>();
    gallery.add<examples::GeometryShowcase>();
    gallery.add<examples::AnimatedModel>();
    gallery.add<examples::PhysicsSandbox>();
    gallery.add<examples::GltfAnimation>();

    examples::Gallery::Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--check")
        {
            // Enough frames that a first draw doing more than the following
            // ones, such as the first upload of a vertex array, is included.
            options.frames = 10u;
            options.visit_all = true;
        }
        else if (argument == "--cycle")
        {
            // Five seconds each, going round for ever. What to leave running on
            // a second screen while working on the library.
            options.frames = 300u;
        }
        else if (argument == "--no-overlay")
        {
            options.overlay = false;
        }
        else if ((argument == "--shots") && ((i + 1) < argc))
        {
            options.screenshots = argv[++i];
        }
        else if ((argument == "--help") || (argument == "-h"))
        {
            std::cout
                << "Usage: " << argv[0] << " [options] [name of an example]\n"
                << "  --check       draw every example for a few frames, then "
                   "report anything\n"
                << "                that failed or was left on the device, and "
                   "exit\n"
                << "  --cycle       move on to the next example every few "
                   "seconds\n"
                << "  --no-overlay  start with the list and the counters "
                   "hidden\n"
                << "  --shots DIR   write one picture per example into DIR, "
                   "which "
                   "needs --check\n"
                << "                or --cycle to say when to take them"
                << std::endl;
            return EXIT_SUCCESS;
        }
        else
        {
            options.start = argument;
        }
    }

    gpu::Status ran = gallery.run(options);
    if (!ran)
    {
        std::cerr << ran.error() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

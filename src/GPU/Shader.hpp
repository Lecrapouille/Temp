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

#pragma once

#include "GPU/Core/Enums.hpp"
#include "GPU/Core/Handle.hpp"
#include "GPU/Core/Reflection.hpp"
#include "GPU/Core/Result.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"

#include <initializer_list>
#include <span>
#include <string>
#include <string_view>

namespace gpu
{

//! \brief Names one compiled stage.
using ShaderHandle = Handle<struct ShaderTag>;

//! \brief Names one linked program.
using ProgramHandle = Handle<struct ProgramTag>;

// ****************************************************************************
//! \brief One stage of a pipeline, compiled and ready to be linked.
//!
//! A stage is worth having as a value of its own for two reasons. A vertex shader
//! shared by several programs is compiled once. And a compilation error is
//! reported on its own, with the log of the one stage that failed, rather than
//! being mixed into a link error that names none of them.
//!
//! \code
//! auto vertex = gpu::Shader::fromFile("shaders/mesh.vert");
//! if (!vertex) { std::cerr << vertex.error(); return; }
//! \endcode
//!
//! Most callers never need this type: Program::fromFiles() does both steps.
// ****************************************************************************
class Shader
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty shader, holding nothing.
    // ------------------------------------------------------------------------
    Shader() = default;

    // ------------------------------------------------------------------------
    //! \brief Compile source code for one stage.
    //!
    //! \param[in] p_stage which stage this is.
    //! \param[in] p_source the GLSL source, which must begin with its #version.
    //! \param[in] p_name what to call it in messages, typically the file it came
    //! from.
    //! \return the compiled stage, or the compiler log as the error. The log is
    //! passed through as the driver wrote it, since its line numbers are what
    //! makes it useful.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromSource(ShaderStage p_stage,
                                                   std::string_view p_source,
                                                   std::string p_name = {});

    // ------------------------------------------------------------------------
    //! \brief Read a file and compile it, taking the stage from the extension.
    //!
    //! Recognises .vert and .vs, .frag and .fs, .geom and .gs, .comp and .cs,
    //! .tesc and .tese. Use the overload taking a stage for anything else.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromFile(std::string const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Read a file and compile it as the given stage.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromFile(ShaderStage p_stage,
                                                 std::string const& p_path);

    Shader(Shader&& p_other) noexcept;
    Shader& operator=(Shader&& p_other) noexcept;
    Shader(Shader const&) = delete;
    Shader& operator=(Shader const&) = delete;
    ~Shader();

    // ------------------------------------------------------------------------
    //! \brief Drop the compiled stage now.
    //!
    //! Safe to do as soon as every program using it has been linked: a program
    //! keeps what it needs.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Which stage this is.
    // ------------------------------------------------------------------------
    [[nodiscard]] ShaderStage stage() const;

    // ------------------------------------------------------------------------
    //! \brief What it is called in messages.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& name() const;

    // ------------------------------------------------------------------------
    //! \brief Is there a compiled stage here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief Name of the compiled stage.
    // ------------------------------------------------------------------------
    [[nodiscard]] ShaderHandle handle() const
    {
        return m_handle;
    }

private:

    explicit Shader(ShaderHandle p_handle) : m_handle(p_handle) {}

    ShaderHandle m_handle;
};

// ****************************************************************************
//! \brief Several stages linked together, and everything they turn out to
//! declare.
//!
//! \code
//! auto program = gpu::Program::fromFiles("shaders/mesh.vert",
//!                                        "shaders/mesh.frag");
//! if (!program)
//! {
//!     std::cerr << program.error() << std::endl;   // the link log
//!     return;
//! }
//! std::cout << program.value().reflection().toString() << std::endl;
//! \endcode
//!
//! Linking is where the driver decides everything the library then relies on:
//! which slot each attribute got, where each member of a uniform block really
//! sits, which binding a storage block expects. All of it is read once, here, and
//! kept: see reflection().
// ****************************************************************************
class Program
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty program.
    // ------------------------------------------------------------------------
    Program() = default;

    // ------------------------------------------------------------------------
    //! \brief Link stages that have already been compiled.
    //!
    //! \param[in] p_stages the stages, which stay usable afterwards and may be
    //! linked into other programs.
    //! \return the linked program, or the linker log as the error.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> link(
        std::initializer_list<Shader const*> p_stages);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from source.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromSources(
        std::string_view p_vertex, std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from files.
    //!
    //! What almost every caller wants. On failure the error names the file that
    //! would not compile, followed by the log of the driver.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromFiles(
        std::string const& p_vertex, std::string const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from source.
    //!
    //! A compute program has one stage and no drawing: it runs on a grid of work
    //! groups whose size the shader itself declares, which reflection() reports
    //! as work_group_size.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromComputeSource(
        std::string_view p_source);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from a file.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromComputeFile(
        std::string const& p_path);

    Program(Program&& p_other) noexcept;
    Program& operator=(Program&& p_other) noexcept;
    Program(Program const&) = delete;
    Program& operator=(Program const&) = delete;
    ~Program();

    // ------------------------------------------------------------------------
    //! \brief Drop the program now rather than at the end of the scope.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Is there a linked program here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief Name of the linked program.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramHandle handle() const
    {
        return m_handle;
    }

    // ------------------------------------------------------------------------
    //! \brief Everything the program declares, as the driver reports it.
    //!
    //! Read once at link time, so asking costs nothing.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramReflection const& reflection() const;

    // ------------------------------------------------------------------------
    //! \brief Set a uniform, checking that the shader declares it with a
    //! matching type.
    //!
    //! \code
    //! GPU_TRY(program.set("uModel", model));
    //! GPU_TRY(program.set("uColor", Vector4f(1.0f, 0.0f, 0.0f, 1.0f)));
    //! GPU_TRY(program.set("uTexture", 0));   // texture unit
    //! \endcode
    //!
    //! \return why it could not be set: no uniform of that name, or one whose
    //! declared type is not this one. Both used to pass unnoticed and show up as
    //! a black screen.
    //!
    //! \note For anything that changes every frame and is shared by several
    //! programs, a uniform block is the better tool: one buffer written once
    //! rather than one call per program per value.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status set(std::string_view p_name, float p_value);
    //! \brief Set a vec2 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Vector2f const& p_value);
    //! \brief Set a vec3 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Vector3f const& p_value);
    //! \brief Set a vec4 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Vector4f const& p_value);
    //! \brief Set an int uniform, or the texture unit of a sampler.
    [[nodiscard]] Status set(std::string_view p_name, int p_value);
    //! \brief Set a uint uniform.
    [[nodiscard]] Status set(std::string_view p_name, unsigned int p_value);
    //! \brief Set a bool uniform.
    [[nodiscard]] Status set(std::string_view p_name, bool p_value);
    //! \brief Set an ivec2 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Vector2i const& p_value);
    //! \brief Set an ivec3 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Vector3i const& p_value);
    //! \brief Set an ivec4 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Vector4i const& p_value);
    //! \brief Set a mat2 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Matrix22f const& p_value);
    //! \brief Set a mat3 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Matrix33f const& p_value);
    //! \brief Set a mat4 uniform.
    [[nodiscard]] Status set(std::string_view p_name, Matrix44f const& p_value);

    // ------------------------------------------------------------------------
    //! \brief Say which binding point a uniform block is to be read from.
    //!
    //! Only needed when the shader did not say so itself with
    //! `layout(binding = 1)`, which is the clearer place to say it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bindUniformBlock(std::string_view p_name,
                                          int p_binding);

private:

    explicit Program(ProgramHandle p_handle) : m_handle(p_handle) {}

    //! \brief Find a uniform and check its declared type before writing to it.
    [[nodiscard]] Result<int> locationOf(std::string_view p_name,
                                         DataType p_expected) const;

    ProgramHandle m_handle;
};

// ----------------------------------------------------------------------------
//! \brief How many compiled stages are alive. Used to spot leaks.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t liveShaders();

// ----------------------------------------------------------------------------
//! \brief How many linked programs are alive. Used to spot leaks.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t livePrograms();

} // namespace gpu

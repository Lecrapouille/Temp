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

#include <cassert>
#include <optional>
#include <string>
#include <utility>

namespace gloop
{

// ****************************************************************************
//! \brief An error carrying its message, on its way up to the caller.
//!
//! This type exists so that a failure can be written once and returned from a
//! function whatever its return type:
//! \code
//! gloop::Result<Texture> load(std::string const& p_path)
//! {
//!     if (!exists(p_path))
//!         return gloop::failure("no such file: " + p_path);
//!     ...
//! }
//! \endcode
// ****************************************************************************
struct Failure
{
    std::string message;
};

// ----------------------------------------------------------------------------
//! \brief Build a failure to be returned as any Result.
//! \param[in] p_message what went wrong, in a form a user can act on.
// ----------------------------------------------------------------------------
[[nodiscard]] inline Failure failure(std::string p_message)
{
    return Failure{ std::move(p_message) };
}

// ****************************************************************************
//! \brief Holds either a value or the reason why it could not be produced.
//!
//! This is the C++20 stand-in for C++23's std::expected, in the spirit of
//! Rust's Result. The library uses it for everything that can fail for a reason
//! outside the programmer's control: compiling a shader, reading a file, asking
//! the driver for a feature it does not have. Exceptions are not used, and
//! mistakes that are the programmer's own fault (using a released handle,
//! reading a value from a failed Result) are assertions instead, because they
//! must be fixed rather than handled.
//!
//! \code
//! auto program = gpu::Program::fromFiles("a.vert", "a.frag");
//! if (!program)
//! {
//!     std::cerr << program.error() << std::endl;
//!     return;
//! }
//! program.value().use();
//! \endcode
//!
//! \tparam T type of the value on success. Use Result<void> when there is
//! nothing to return but the operation can still fail.
// ****************************************************************************
template <typename T>
class [[nodiscard]] Result
{
public:

    Result(T p_value) : m_value(std::move(p_value)) {}

    Result(Failure p_failure) : m_error(std::move(p_failure.message)) {}

    [[nodiscard]] explicit operator bool() const
    {
        return m_value.has_value();
    }

    [[nodiscard]] T& value()
    {
        assert(m_value.has_value() && "Result::value() on a failed Result");
        return *m_value;
    }

    [[nodiscard]] T const& value() const
    {
        assert(m_value.has_value() && "Result::value() on a failed Result");
        return *m_value;
    }

    [[nodiscard]] T take()
    {
        assert(m_value.has_value() && "Result::take() on a failed Result");
        return std::move(*m_value);
    }

    [[nodiscard]] T valueOr(T p_fallback) const
    {
        return m_value.has_value() ? *m_value : std::move(p_fallback);
    }

    [[nodiscard]] std::string const& error() const
    {
        return m_error;
    }

private:

    std::optional<T> m_value;
    std::string m_error;
};

// ****************************************************************************
//! \brief A Result with nothing to return: it either worked or it did not.
// ****************************************************************************
template <>
class [[nodiscard]] Result<void>
{
public:

    Result() = default;

    Result(Failure p_failure)
        : m_error(std::move(p_failure.message)), m_ok(false)
    {
    }

    [[nodiscard]] explicit operator bool() const
    {
        return m_ok;
    }

    [[nodiscard]] std::string const& error() const
    {
        return m_error;
    }

private:

    std::string m_error;
    bool m_ok = true;
};

//! \brief Shorthand for a Result carrying no value.
using Status = Result<void>;

[[nodiscard]] inline Status success()
{
    return Status{};
}

} // namespace gloop

// ****************************************************************************
//! \brief Run an expression returning a Result and give up early if it failed,
//! forwarding the error to our own caller.
// ****************************************************************************
#define GPU_TRY(expr)                                        \
    do                                                       \
    {                                                        \
        auto gpu_try_result_ = (expr);                       \
        if (!gpu_try_result_)                                \
        {                                                    \
            return gloop::failure(gpu_try_result_.error());  \
        }                                                    \
    } while (false)

// ****************************************************************************
//! \brief Same as GPU_TRY, but binds the value to a new variable on success.
// ****************************************************************************
#define GPU_TRY_ASSIGN(var, expr)                        \
    auto var##_result_ = (expr);                         \
    if (!var##_result_)                                  \
    {                                                    \
        return gloop::failure(var##_result_.error());    \
    }                                                    \
    auto var = var##_result_.take()

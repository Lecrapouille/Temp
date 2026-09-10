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

#include "GPU/Core/Layout.hpp"

#include <algorithm>

namespace gpu
{

//------------------------------------------------------------------------------
std::string AttributeFormat::glslType() const
{
    const std::string count = std::to_string(components);

    // Several slots means a matrix, which the hardware reads one column per slot.
    // GLSL names it by columns first, so a field spanning four slots of four
    // components is a mat4, and one spanning three slots of four is a mat3x4.
    if (slots > 1u)
    {
        const std::string prefix =
            (scalar == ScalarType::Double) ? "dmat" : "mat";
        if (slots == components)
        {
            return prefix + std::to_string(slots);
        }
        return prefix + std::to_string(slots) + "x" + count;
    }

    if (components == 1u)
    {
        if (normalized || !as_integer)
        {
            return (scalar == ScalarType::Double) ? "double" : "float";
        }
        return (scalar == ScalarType::UInt8) || (scalar == ScalarType::UInt16) ||
                       (scalar == ScalarType::UInt32)
                   ? "uint"
                   : "int";
    }

    if (normalized || !as_integer)
    {
        return ((scalar == ScalarType::Double) ? "dvec" : "vec") + count;
    }
    const bool unsigned_kind = (scalar == ScalarType::UInt8) ||
                               (scalar == ScalarType::UInt16) ||
                               (scalar == ScalarType::UInt32);
    return (unsigned_kind ? "uvec" : "ivec") + count;
}

//------------------------------------------------------------------------------
VertexLayout::VertexLayout(std::vector<FieldDesc> p_fields,
                           std::uint32_t p_stride)
    : m_fields(std::move(p_fields)), m_stride(p_stride)
{
    check();
}

//------------------------------------------------------------------------------
FieldDesc const* VertexLayout::find(std::string_view p_name) const
{
    auto it = std::find_if(
        m_fields.begin(), m_fields.end(), [&](FieldDesc const& p_field) {
            return p_field.name == p_name;
        });
    return (it == m_fields.end()) ? nullptr : &(*it);
}

//------------------------------------------------------------------------------
std::size_t VertexLayout::slots() const
{
    std::size_t total = 0u;
    for (auto const& field : m_fields)
    {
        total += field.format.slots;
    }
    return total;
}

//------------------------------------------------------------------------------
bool VertexLayout::hasPerInstanceFields() const
{
    return std::any_of(
        m_fields.begin(), m_fields.end(), [](FieldDesc const& p_field) {
            return p_field.per_instance;
        });
}

//------------------------------------------------------------------------------
Status VertexLayout::validate() const
{
    if (m_error.empty())
    {
        return success();
    }
    return failure(m_error);
}

//------------------------------------------------------------------------------
// Everything checked here is knowable without a shader. The point is to fail
// while the reader is still looking at the layout, rather than to draw a wrong
// image and leave them wondering.
//------------------------------------------------------------------------------
void VertexLayout::check()
{
    if (m_fields.empty())
    {
        return;
    }

    if (m_stride == 0u)
    {
        m_error = "a vertex layout with fields cannot have a stride of zero";
        return;
    }

    for (std::size_t i = 0u; i < m_fields.size(); ++i)
    {
        FieldDesc const& field = m_fields[i];

        if (field.name.empty())
        {
            m_error = "field " + std::to_string(i) + " has no name, so no "
                      "shader attribute can be matched to it";
            return;
        }

        if (!field.format.valid())
        {
            m_error = "field '" + field.name + "' has a format the hardware "
                      "cannot read: " +
                      std::to_string(field.format.components) +
                      " components of " + gpu::toString(field.format.scalar) +
                      " over " + std::to_string(field.format.slots) +
                      " slots. Between 1 and 4 components are allowed, and a "
                      "field cannot be both normalized and read as a whole "
                      "number";
            return;
        }

        const std::size_t end = field.offset + field.format.size();
        if (end > m_stride)
        {
            m_error = "field '" + field.name + "' ends at byte " +
                      std::to_string(end) + " but a vertex is only " +
                      std::to_string(m_stride) +
                      " bytes long. Is the field described as belonging to the "
                      "wrong struct?";
            return;
        }

        for (std::size_t j = i + 1u; j < m_fields.size(); ++j)
        {
            FieldDesc const& other = m_fields[j];

            if (other.name == field.name)
            {
                m_error = "two fields are both named '" + field.name +
                          "'. A shader attribute would not know which one to "
                          "read";
                return;
            }

            const std::size_t other_end = other.offset + other.format.size();
            const bool overlap =
                (field.offset < other_end) && (other.offset < end);
            if (overlap)
            {
                m_error = "fields '" + field.name + "' at byte " +
                          std::to_string(field.offset) + " and '" + other.name +
                          "' at byte " + std::to_string(other.offset) +
                          " overlap. Describing the same member twice is the "
                          "usual cause";
                return;
            }
        }
    }
}

//------------------------------------------------------------------------------
std::string VertexLayout::toString() const
{
    if (m_fields.empty())
    {
        return "no vertex data";
    }

    std::string text = "vertex of " + std::to_string(m_stride) + " bytes:";
    for (auto const& field : m_fields)
    {
        text += "\n  " + field.format.glslType() + " " + field.name +
                " at byte " + std::to_string(field.offset) + " (" +
                std::to_string(field.format.components) + " x " +
                gpu::toString(field.format.scalar);
        if (field.format.normalized)
        {
            text += ", normalized";
        }
        if (field.per_instance)
        {
            text += ", per instance";
        }
        text += ")";
    }
    return text;
}

} // namespace gpu

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

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "Common/ImGuiTheme.hpp"
#include "Common/Screenshot.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>

namespace examples
{

namespace
{

// ----------------------------------------------------------------------------
//! \brief What is left over between two counts, as a sentence, or nothing at all
//! when the two agree.
//!
//! The whole reason the gallery keeps the counters: closing an example has to
//! bring the device back to where it was. Anything that stayed behind is named
//! here rather than being left for a memory tool to find later.
// ----------------------------------------------------------------------------
std::string whatStayedBehind(gpu::ResourceStatistics const& p_before,
                             gpu::ResourceStatistics const& p_after)
{
    std::string left;
    auto note = [&left](char const* p_what, std::size_t p_before_count,
                        std::size_t p_after_count)
    {
        if (p_after_count > p_before_count)
        {
            if (!left.empty())
            {
                left += ", ";
            }
            left += std::to_string(p_after_count - p_before_count);
            left += " ";
            left += p_what;
        }
    };

    note("buffers", p_before.buffers, p_after.buffers);
    note("textures", p_before.textures, p_after.textures);
    note("shaders", p_before.shaders, p_after.shaders);
    note("programs", p_before.programs, p_after.programs);
    note("pipelines", p_before.pipelines, p_after.pipelines);
    note("framebuffers", p_before.framebuffers, p_after.framebuffers);
    note("vertex readers", p_before.vertex_readers, p_after.vertex_readers);

    return left;
}

//------------------------------------------------------------------------------
//! \brief A count with the count of the gallery itself taken off, so that the
//! overlay says what this example is using rather than what the process is using.
//------------------------------------------------------------------------------
std::size_t owed(std::size_t p_now, std::size_t p_before)
{
    return (p_now > p_before) ? (p_now - p_before) : 0u;
}

} // namespace

//------------------------------------------------------------------------------
void Gallery::add(std::string p_name, Factory p_factory)
{
    m_entries.push_back(Entry{ std::move(p_name), std::move(p_factory) });
}

//------------------------------------------------------------------------------
Gallery::~Gallery()
{
    // The example goes first: it holds device resources, and the context it holds
    // them on belongs to the window, which is destroyed after this body runs.
    m_current.reset();
}

//------------------------------------------------------------------------------
void Gallery::hide()
{
    if (m_current == nullptr)
    {
        return;
    }

    const std::string closed = m_entries[m_index].name;
    m_current.reset();

    const gpu::ResourceStatistics after = gpu::resourceStatistics();
    m_leak = whatStayedBehind(m_before, after);
    if (!m_leak.empty())
    {
        m_problems.push_back(closed + " left " + m_leak +
                             " on the device after being closed");
        std::cerr << "Leak: closing " << closed << " left " << m_leak
                  << " on the device" << std::endl;
    }
}

//------------------------------------------------------------------------------
void Gallery::show(std::size_t p_index)
{
    hide();

    m_failure.clear();
    m_frames = 0u;
    m_index = p_index;
    if (m_index >= m_entries.size())
    {
        return;
    }

    // Read before the example is built and not after, so that what the overlay
    // shows is what this example added.
    m_before = gpu::resourceStatistics();

    m_current = m_entries[m_index].factory();
    if (m_current == nullptr)
    {
        m_failure = "this example could not be created at all";
        m_problems.push_back(m_entries[m_index].name + " could not be created");
        return;
    }

    m_started = m_now;
    if (m_announce)
    {
        // On the error stream, next to what the driver says: the two have to be read
        // together, and a stream that is buffered differently would print them out
        // of order.
        std::cerr << "Showing " << m_entries[m_index].name << std::endl;
    }

    gpu::Status ready = m_current->setUp();
    if (!ready)
    {
        m_failure = ready.error();
        // Dropped rather than kept in a half built state: an example that could
        // not set itself up has no business being asked to draw sixty times a
        // second, and the reason stays on the screen.
        m_current.reset();
        m_problems.push_back(m_entries[m_index].name + " could not set up: " +
                             m_failure);
        std::cerr << "Cannot run " << m_entries[m_index].name << ": " << m_failure
                  << std::endl;
    }
}

//------------------------------------------------------------------------------
gpu::Status Gallery::run(Options const& p_options)
{
    if (m_entries.empty())
    {
        return gpu::failure("the gallery has no examples in it");
    }

    m_show_overlay = p_options.overlay;
    m_announce = (p_options.frames != 0u);

    GPU_TRY(m_window.open("OpenGLCppWrapper", 1024, 768));

    // Dear ImGui talks to the same context we do, with its own shaders and its own
    // textures. Two consequences worth knowing before reading the loop below: our
    // record of the device state has to be thrown away after it has drawn, and the
    // driver messages the library forwards include complaints about its drawing as
    // well as ours. The ones about a texture unit with nothing bound on the frame
    // its font atlas appears are its, not ours.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imgui_io = ImGui::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ui::applyGalleryTheme();
    imgui_io.IniFilename = "examples/imgui.ini";
    ImGui_ImplGlfw_InitForOpenGL(m_window.handle(), true);
    // The version the shaders of the overlay are written against, not the version
    // of the context: 130 works on everything from 3.0 up.
    ImGui_ImplOpenGL3_Init("#version 130");

    std::size_t start = 0u;
    if (!p_options.start.empty())
    {
        bool found = false;
        for (std::size_t i = 0u; i < m_entries.size(); ++i)
        {
            if (m_entries[i].name == p_options.start)
            {
                start = i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cerr << "No example is called " << p_options.start
                      << ", starting at the first one instead" << std::endl;
        }
    }
    show(start);

    //! How many examples are left to visit when the gallery walks the list itself.
    std::size_t remaining = m_entries.size();

    while (!m_window.closing())
    {
        m_window.beginFrame();
        m_now += m_window.elapsed();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (m_current != nullptr)
        {
            Frame frame;
            frame.width = m_window.width();
            frame.height = m_window.height();
            frame.elapsed = m_window.elapsed();
            frame.total = m_now - m_started;
            frame.mouse = m_window.mouse();

            ImGuiIO const& io = ImGui::GetIO();
            if (!io.WantCaptureMouse)
            {
                frame.mouse_delta = m_window.mouseDelta();
                frame.scroll = m_window.scroll();
                frame.mouse_left = m_window.mouseLeft();
                frame.mouse_right = m_window.mouseRight();
                frame.mouse_left_pressed = m_window.mouseLeftPressed();
            }
            if (!io.WantCaptureKeyboard)
            {
                frame.key_w = m_window.keyDown(GLFW_KEY_W);
                frame.key_a = m_window.keyDown(GLFW_KEY_A);
                frame.key_s = m_window.keyDown(GLFW_KEY_S);
                frame.key_d = m_window.keyDown(GLFW_KEY_D);
                frame.key_q = m_window.keyDown(GLFW_KEY_Q);
                frame.key_e = m_window.keyDown(GLFW_KEY_E);
                frame.key_shift = m_window.keyDown(GLFW_KEY_LEFT_SHIFT) ||
                                  m_window.keyDown(GLFW_KEY_RIGHT_SHIFT);
                frame.key_1 = m_window.keyDown(GLFW_KEY_1);
                frame.key_2 = m_window.keyDown(GLFW_KEY_2);
                frame.key_3 = m_window.keyDown(GLFW_KEY_3);
            }

            gpu::Status drawn = m_current->draw(frame);
            ++m_frames;
            if (!drawn)
            {
                m_failure = drawn.error();
                m_current.reset();
                m_problems.push_back(m_entries[m_index].name +
                                     " stopped while drawing: " + m_failure);
                std::cerr << "Stopped " << m_entries[m_index].name << ": "
                          << m_failure << std::endl;
            }
        }
        else
        {
            // Counted even though nothing was drawn, so that a run walking the list
            // does not stall for ever on an example that failed to set itself up.
            ++m_frames;

            // Nothing is running, either because an example failed or because the
            // list ran out. The screen is still cleared, so that what is left on it
            // is not the last frame of something that no longer exists.
            gpu::PassDesc desc;
            desc.width = m_window.width();
            desc.height = m_window.height();
            desc.color = Vector4f(0.1f, 0.1f, 0.12f, 1.0f);
            gpu::Result<gpu::RenderPass> pass = gpu::RenderPass::begin(desc);
            if (!pass)
            {
                std::cerr << "Cannot even clear the screen: " << pass.error()
                          << std::endl;
            }
        }

        if (m_show_overlay)
        {
            drawUi();
        }

        // Render() is called even when the overlay is hidden, because it is what
        // ends the frame ImGui started; only the drawing of it is skipped.
        ImGui::Render();
        if (m_show_overlay)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // The overlay is drawn with plain OpenGL calls of its own, behind our
            // back and without asking. Whatever state it left is not what our own
            // cache believes, so the cache is told to stop believing anything. This
            // is the one line that keeps a library sharing our context from turning
            // the next frame into a puzzle.
            gpu::forgetRenderState();
        }

        // Read after the overlay has been dealt with, so that a key pressed on a
        // text field of the overlay does not also switch the example.
        ImGuiIO const& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard)
        {
            const bool next = ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) ||
                              ImGui::IsKeyPressed(ImGuiKey_PageDown, false);
            const bool previous = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) ||
                                  ImGui::IsKeyPressed(ImGuiKey_PageUp, false);

            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            {
                m_window.close();
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
            {
                m_show_overlay = !m_show_overlay;
            }
            else if (next)
            {
                show((m_index + 1u) % m_entries.size());
            }
            else if (previous)
            {
                show((m_index + m_entries.size() - 1u) % m_entries.size());
            }
        }

        if (!p_options.screenshots.empty() && (p_options.frames != 0u) &&
            (m_frames == p_options.frames))
        {
            saveScreenshot(p_options.screenshots);
        }

        m_window.endFrame();

        if ((p_options.frames != 0u) && (m_frames >= p_options.frames))
        {
            --remaining;
            if (p_options.visit_all && (remaining == 0u))
            {
                m_window.close();
            }
            else
            {
                show((m_index + 1u) % m_entries.size());
            }
        }
    }

    hide();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (!m_problems.empty())
    {
        std::string why = std::to_string(m_problems.size()) +
                          " example(s) did not behave:";
        for (std::string const& problem : m_problems)
        {
            why += "\n  " + problem;
        }
        return gpu::failure(why);
    }

    return gpu::success();
}

//------------------------------------------------------------------------------
void Gallery::saveScreenshot(std::string const& p_directory)
{
    if (m_index >= m_entries.size())
    {
        return;
    }

    // Reading happens through a pass, since a pass is what says which target is
    // being worked on. This one clears nothing: it is opened over what the example
    // and the overlay have just drawn, to read it back.
    gpu::PassDesc whole;
    whole.width = m_window.width();
    whole.height = m_window.height();
    whole.clear_color = false;
    whole.clear_depth = false;

    gpu::Result<gpu::RenderPass> pass = gpu::RenderPass::begin(whole);
    if (!pass)
    {
        std::cerr << "Cannot look at the screen: " << pass.error() << std::endl;
        return;
    }

    gpu::Result<std::vector<std::byte>> picture = gpu::readPixels();
    if (!picture)
    {
        std::cerr << "Cannot read the screen: " << picture.error() << std::endl;
        return;
    }

    std::error_code failed;
    std::filesystem::create_directories(p_directory, failed);
    if (failed)
    {
        std::cerr << "Cannot write into " << p_directory << ": "
                  << failed.message() << std::endl;
        return;
    }

    const std::string path =
        p_directory + "/" + m_entries[m_index].name + ".png";

    gpu::Status saved = writePng(
        path, m_window.width(), m_window.height(), picture.value());
    if (!saved)
    {
        std::cerr << saved.error() << std::endl;
    }
    else
    {
        std::cerr << "Wrote " << path << std::endl;
    }
}

namespace
{

constexpr float STATUS_BAR_HEIGHT = 26.0f;

} // namespace

//------------------------------------------------------------------------------
char const* Gallery::categoryOf(std::string const& p_name)
{
    const std::size_t sep = p_name.find('_');
    if (sep == std::string::npos)
    {
        return "Other";
    }
    const int id = std::stoi(p_name.substr(0u, sep));
    if (id == 30)
    {
        return "Scientific";
    }
    if ((id <= 8) || ((id >= 28) && (id <= 32)))
    {
        return "Basics";
    }
    if (id <= 12)
    {
        return "Scientific";
    }
    if (id <= 14)
    {
        return "Compute";
    }
    if (id <= 16)
    {
        return "Performance";
    }
    return "World";
}

//------------------------------------------------------------------------------
void Gallery::drawDockspace()
{
    ImGuiViewport const* const viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::Begin("GalleryMenuBar", nullptr, flags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Esc"))
            {
                m_window.close();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Show panels", "Space", m_show_overlay))
            {
                m_show_overlay = !m_show_overlay;
            }
            if (ImGui::MenuItem("Reset layout"))
            {
                m_reset_layout = true;
                m_build_default_layout = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Examples"))
        {
            if (ImGui::MenuItem("Previous", "Left / PgUp"))
            {
                show((m_index + m_entries.size() - 1u) % m_entries.size());
            }
            if (ImGui::MenuItem("Next", "Right / PgDn"))
            {
                show((m_index + 1u) % m_entries.size());
            }
            ImGui::Separator();
            for (std::size_t i = 0u; i < m_entries.size(); ++i)
            {
                const bool chosen = (i == m_index);
                if (ImGui::MenuItem(m_entries[i].name.c_str(), nullptr, chosen))
                {
                    show(i);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::TextDisabled("Arrows: change example");
            ImGui::TextDisabled("Space: hide panels");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawExamplesPanel()
{
    ImGuiViewport const* const viewport = ImGui::GetMainViewport();
    const float top = ImGui::GetFrameHeight();
    if (m_build_default_layout || m_reset_layout)
    {
        ImGui::SetNextWindowPos(
            ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + top),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(viewport->WorkSize.x * 0.24f,
                   viewport->WorkSize.y - top - STATUS_BAR_HEIGHT),
            ImGuiCond_Always);
    }
    else
    {
        ImGui::SetNextWindowSize(ImVec2(280.0f, 480.0f), ImGuiCond_FirstUseEver);
    }
    if (!ImGui::Begin("Examples", &m_show_overlay))
    {
        ImGui::End();
        return;
    }

    ImGui::SetNextItemWidth(-1.0f);
    static char filter[64] = {};
    ImGui::InputTextWithHint("##filter", "Filter...", filter, sizeof(filter));

    char const* open_category = nullptr;
    bool category_open = false;
    for (std::size_t i = 0u; i < m_entries.size(); ++i)
    {
        const std::string& name = m_entries[i].name;
        if ((filter[0] != '\0') &&
            (name.find(filter) == std::string::npos))
        {
            continue;
        }

        char const* const category = categoryOf(name);
        if (open_category != category)
        {
            if (category_open)
            {
                ImGui::TreePop();
            }
            open_category = category;
            ImGui::PushID(category);
            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            category_open =
                ImGui::TreeNodeEx(category, ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopID();
        }

        if (!category_open)
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));
        const bool chosen = (i == m_index);
        if (ImGui::Selectable(name.c_str(), chosen))
        {
            show(i);
        }
        if (chosen)
        {
            ImGui::SetItemDefaultFocus();
        }
        ImGui::PopID();
    }
    if (category_open)
    {
        ImGui::TreePop();
    }

    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawInspectorPanel()
{
    ImGuiViewport const* const viewport = ImGui::GetMainViewport();
    const float top = ImGui::GetFrameHeight();
    if (m_build_default_layout || m_reset_layout)
    {
        ImGui::SetNextWindowPos(
            ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.68f,
                   viewport->WorkPos.y + top),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(viewport->WorkSize.x * 0.32f,
                   viewport->WorkSize.y - top - STATUS_BAR_HEIGHT),
            ImGuiCond_Always);
    }
    else
    {
        ImGui::SetNextWindowSize(ImVec2(360.0f, 480.0f), ImGuiCond_FirstUseEver);
    }
    if (!ImGui::Begin("Inspector", &m_show_overlay))
    {
        ImGui::End();
        return;
    }

    if (m_index < m_entries.size())
    {
        ImGui::TextUnformatted(m_entries[m_index].name.c_str());
        ImGui::Separator();
    }

    if (m_current != nullptr)
    {
        ImGui::TextWrapped("%s", m_current->description().c_str());
    }
    if (!m_failure.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("Not running: %s", m_failure.c_str());
        ImGui::PopStyleColor();
    }
    if (!m_leak.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));
        ImGui::TextWrapped("A previous example left %s behind.", m_leak.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::Text("Frame");
    gpu::FrameStatistics const& frame = gpu::frameStatistics();
    ImGui::Text("%.1f FPS  %.2f ms", m_window.framesPerSecond(),
                static_cast<double>(m_window.elapsed()) * 1000.0);
    ImGui::Text("Draws %zu  Passes %zu  Dispatches %zu", frame.draw_calls,
                frame.passes, frame.dispatches);
    ImGui::Text("Vertices %zu  Instances %zu", frame.vertices, frame.instances);

    ImGui::Separator();
    ImGui::Text("Device (this example)");
    const gpu::ResourceStatistics now = gpu::resourceStatistics();
    ImGui::Text("Buffers %zu  Textures %zu",
                owed(now.buffers, m_before.buffers),
                owed(now.textures, m_before.textures));
    ImGui::Text("Programs %zu  Pipelines %zu  Readers %zu",
                owed(now.programs, m_before.programs),
                owed(now.pipelines, m_before.pipelines),
                owed(now.vertex_readers, m_before.vertex_readers));
    ImGui::Text("Buffer mem %zu KiB  Texture mem %zu KiB",
                owed(now.buffer_bytes, m_before.buffer_bytes) / 1024u,
                owed(now.texture_bytes, m_before.texture_bytes) / 1024u);

    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawStatusBar()
{
    ImGuiViewport const* const viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x,
               viewport->WorkPos.y + viewport->WorkSize.y - STATUS_BAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, STATUS_BAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 4.0f));
    if (ImGui::Begin("##GalleryStatusBar", nullptr, flags))
    {
        if (m_index < m_entries.size())
        {
            ImGui::Text("%s", m_entries[m_index].name.c_str());
            ImGui::SameLine(0.0f, 16.0f);
        }
        ImGui::Text("%.1f fps", m_window.framesPerSecond());
        ImGui::SameLine(0.0f, 16.0f);
        ImGui::TextDisabled("Space: panels  Esc: quit  Arrows: navigate");
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
}

//------------------------------------------------------------------------------
void Gallery::drawUi()
{
    drawDockspace();
    drawExamplesPanel();
    drawInspectorPanel();
    drawStatusBar();
    if (m_reset_layout)
    {
        m_reset_layout = false;
        m_build_default_layout = false;
    }
}

} // namespace examples

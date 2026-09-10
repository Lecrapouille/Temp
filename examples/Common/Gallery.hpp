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

#include "Common/Example.hpp"
#include "Common/Window.hpp"

#include <functional>
#include <string>
#include <vector>

// ****************************************************************************
//! \file
//! \brief One program holding every example, switched without restarting.
//!
//! There is one binary rather than twenty, for a reason that is not laziness:
//! switching from one example to the next inside a live device is what proves the
//! resources of the one being left are actually given back. The overlay shows the
//! counters of the device next to the counters of the frame, so a leak shows up as
//! a number that does not come back down when an example is closed.
//!
//! An example is built only when it is chosen. Nothing is on the device for the
//! examples nobody is looking at.
// ****************************************************************************

namespace examples
{

// ****************************************************************************
//! \brief Every example, with the one being shown.
// ****************************************************************************
class Gallery
{
public:

    //! \brief How an example is built, when and only when it is asked for.
    using Factory = std::function<ExamplePtr()>;

    // ************************************************************************
    //! \brief How the gallery is meant to run this time.
    // ************************************************************************
    struct Options
    {
        //! \brief Which example to show first, by name. Empty for the first in the
        //! list. What lets a build script open the gallery straight on the example
        //! someone is working on.
        std::string start;

        //! \brief How many frames to give each example before moving on, or zero to
        //! stay on it until someone chooses another.
        std::size_t frames = 0u;

        //! \brief Close the window once every example has had its turn.
        //!
        //! With frames set, this turns the gallery into a test: every example is
        //! built, drawn, and closed, and anything it failed to give back is
        //! reported. Worth running before a commit, and the reason the examples are
        //! one program rather than twenty.
        bool visit_all = false;

        //! \brief Show the list and the counters.
        bool overlay = true;

        //! \brief Where to write one picture per example, or empty to write none.
        //!
        //! Taken on the last frame an example is given, which is why it goes with
        //! frames rather than with a key to press. It is how the pictures in the
        //! documentation are made, and it is a stronger check than "it did not
        //! crash": a black picture means the example drew nothing.
        std::string screenshots;
    };

    ~Gallery();

    // ------------------------------------------------------------------------
    //! \brief Add an example to the list, without building it.
    //!
    //! \param[in] p_name what the list shows.
    //! \param[in] p_factory what builds it, later.
    // ------------------------------------------------------------------------
    void add(std::string p_name, Factory p_factory);

    // ------------------------------------------------------------------------
    //! \brief Add an example of a known type, by naming the type.
    //!
    //! \code
    //! gallery.add<ClearScreen>();
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename T>
    void add()
    {
        // Built once here, thrown away immediately: the name and the sentence come
        // from the example itself rather than being repeated at the call site,
        // where they would drift apart from it. This costs nothing because an
        // example puts nothing on the device until setUp().
        T probe;
        add(probe.name(), []() { return ExamplePtr(new T()); });
    }

    // ------------------------------------------------------------------------
    //! \brief Open a window and show examples until the user closes it.
    //!
    //! \param[in] p_options where to start, and whether to walk through the whole
    //! list on its own.
    //! \return why the gallery could not run. An example failing does not end the
    //! run: the reason is shown and another can be chosen, but it is remembered and
    //! reported here, so that a run through the whole list is worth something to a
    //! build script.
    // ------------------------------------------------------------------------
    [[nodiscard]] gpu::Status run(Options const& p_options);

    // ------------------------------------------------------------------------
    //! \brief Show examples until the user closes the window, starting at the
    //! first.
    //!
    //! An overload rather than a default argument, because a nested struct is not
    //! complete enough for its own defaults to be used inside the class that holds
    //! it.
    // ------------------------------------------------------------------------
    [[nodiscard]] gpu::Status run()
    {
        return run(Options{});
    }

private:

    // ------------------------------------------------------------------------
    //! \brief What the list holds for each example.
    // ------------------------------------------------------------------------
    struct Entry
    {
        std::string name;
        Factory factory;
    };

    // ------------------------------------------------------------------------
    //! \brief Close whatever is running and build the one at this position.
    // ------------------------------------------------------------------------
    void show(std::size_t p_index);

    // ------------------------------------------------------------------------
    //! \brief Give back what the current example holds, and check it did.
    // ------------------------------------------------------------------------
    void hide();

    // ------------------------------------------------------------------------
    //! \brief Dear ImGui shell: dockspace, browser, inspector, status bar.
    // ------------------------------------------------------------------------
    void drawUi();
    void drawDockspace();
    void drawExamplesPanel();
    void drawInspectorPanel();
    void drawStatusBar();

    [[nodiscard]] static char const* categoryOf(std::string const& p_name);

    // ------------------------------------------------------------------------
    //! \brief Write what is on the screen to a file named after the example.
    //!
    //! \param[in] p_directory where to write it. Created if it is not there.
    // ------------------------------------------------------------------------
    void saveScreenshot(std::string const& p_directory);

    std::vector<Entry> m_entries;
    Window m_window;

    ExamplePtr m_current;
    std::size_t m_index = 0u;
    //! \brief How many frames the current example has been given, for the runs that
    //! move on by themselves.
    std::size_t m_frames = 0u;
    //! \brief Every example that failed, and what it said. Empty after a good run
    //! through the whole list, which is the point of collecting them.
    std::vector<std::string> m_problems;
    //! \brief What the current example said when it stopped working, if it did.
    //! Kept so that the reason stays on the screen instead of scrolling past in a
    //! terminal.
    std::string m_failure;
    //! \brief When the current example was set up, so that its animations start
    //! from zero.
    float m_started = 0.0f;
    float m_now = 0.0f;

    //! \brief What the device was holding before the current example was set up.
    //! What the counters are read against: the interesting number is what this
    //! example added, not what the gallery itself uses.
    gpu::ResourceStatistics m_before;
    //! \brief What was still held after the previous example was closed, if
    //! anything was. Shown as a warning, because that is a leak and the gallery is
    //! where it is easiest to notice.
    std::string m_leak;

    bool m_show_overlay = true;
    bool m_build_default_layout = true;
    bool m_reset_layout = false;
    //! \brief Say on the terminal which example is being shown. Off while somebody
    //! is watching the window, since the list is on the screen; on when the gallery
    //! is walking the list on its own, where the terminal is all there is to read
    //! and a message from the driver has to be attributable to an example.
    bool m_announce = false;
};

} // namespace examples

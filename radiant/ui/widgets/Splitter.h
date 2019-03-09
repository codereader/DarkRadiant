#pragma once

#include <wx/splitter.h>

namespace ui
{

/// Subclass of wxSplitterWindow that provides splitter position persistence
class Splitter
: public wxSplitterWindow
{
    const std::string _registryKey;

private:
    void onPositionChange(wxSplitterEvent&);
    int sashPositionMax();

public:

    /**
     * \brief Construct and initialise
     *
     * \param registryKey
     * Registry key to persist the position of the splitter sash
     */
    Splitter(wxWindow* parent, const std::string& registryKey,
             long style = wxSP_3D, const wxString& name = "Splitter");

    /**
     * \brief
     * Set up the connection to the registry and set the initial sash position
     *
     * This needs to be in a separate method, rather than some override of
     * SplitHorizontally or SplitVertically, to avoid sash positions shifting
     * when the Splitter is reparented or resized during the GUI construction
     * process. To avoid the registry value being overwritten during the window
     * setup, the connection between the position-changed event and the
     * persistence code is not enabled until this method is invoked.
     */
    void connectToRegistry();
};

}

#include "Splitter.h"

#include "registry/registry.h"

#include <wx/event.h>

namespace ui
{

Splitter::Splitter(wxWindow* parent, const std::string& registryKey, long style,
                   const wxString& name)
: wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style, name),
  _registryKey(registryKey)
{
    SetMinimumPaneSize(1); // disallow unsplitting
    SetSashGravity(0.5);
}

int Splitter::sashPositionMax()
{
    // In wxWidgets parlance "split vertically" means drawing the sash
    // vertically, with the content panes on the left and right.
    wxSize size = GetSize();
    return GetSplitMode() == wxSPLIT_VERTICAL ? size.GetWidth()
                                              : size.GetHeight();
}

void Splitter::connectToRegistry()
{
    float sashPosition = registry::getValue<float>(_registryKey, 0.5) * sashPositionMax();
    SetSashPosition(int(sashPosition));

    Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &Splitter::onPositionChange, this);
}

void Splitter::onPositionChange(wxSplitterEvent& e)
{
    e.Skip();

    float position = GetSashPosition() * 1.0f / sashPositionMax();
    registry::setValue(_registryKey, position);
}

}

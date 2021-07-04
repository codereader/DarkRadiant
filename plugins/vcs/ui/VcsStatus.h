#pragma once

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

namespace vcs
{

namespace ui
{

class VcsStatus :
    public wxPanel
{
public:
    static constexpr const char* const Name = "VcsStatusBarWidget";

    VcsStatus(wxWindow* parent) :
        wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER | wxWANTS_CHARS, "VcsStatusBarPanel")
    {
        SetSizer(new wxBoxSizer(wxHORIZONTAL));

        auto* text = new wxStaticText(this, wxID_ANY, "Statüs");
        GetSizer()->Add(text);
    }
};

}

}

#pragma once

#include <wx/popupwin.h>
#include <wx/window.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include "MouseToolManager.h"

namespace ui
{

class ModifierHintPopup :
	public wxPopupWindow
{
private:
	wxStaticText* _text;
    MouseToolManager& _owner;

public:
    ModifierHintPopup(wxWindow* parent, MouseToolManager& owner) :
		wxPopupWindow(parent, wxBORDER_SIMPLE),
        _owner(owner)
	{
		SetSizer(new wxBoxSizer(wxHORIZONTAL));

        _text = new wxStaticText(this, wxID_ANY, "");
        GetSizer()->Add(_text, 1, wxALL, 3);
        SetText("");

        // Show the window right now
        Show();

        // Subscribe to the parent events after showing, to prevent immediate self-destruction
        parent->Bind(wxEVT_ICONIZE, &ModifierHintPopup::_onParentMinimized, this);
        // Detect when the parent window is losing focus (e.g. by alt-tabbing)
        parent->Bind(wxEVT_ACTIVATE, &ModifierHintPopup::_onParentActivate, this);
	}
    
	void SetText(const wxString& text)
	{
        _text->SetLabelText(text);

        Layout();
        Fit();

        Reposition();
	}

private:
	void Reposition()
	{
		// Position this control in the bottom left corner
		wxPoint popupPos = GetParent()->GetScreenPosition() + wxSize(20, GetParent()->GetSize().y - GetSize().y - 20);
		Position(popupPos, wxSize(0, 0));
	}

    void _onParentActivate(wxActivateEvent& ev)
    {
        if (!ev.GetActive())
        {
            _owner.closeHintPopup();
        }
    }

    void _onParentMinimized(wxIconizeEvent&)
    {
        _owner.closeHintPopup();
    }
};

}

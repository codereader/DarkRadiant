#pragma once

#include <wx/popupwin.h>

namespace ui
{

class ModifierHintPopup :
	public wxPopupWindow
{
private:
	wxStaticText* _text;

public:
    ModifierHintPopup(wxWindow* parent) :
		wxPopupWindow(parent, wxBORDER_SIMPLE)
	{
		SetSizer(new wxBoxSizer(wxHORIZONTAL));

        _text = new wxStaticText(this, wxID_ANY, _("Test"));
        GetSizer()->Add(_text, 1, wxALL, 3);

		Layout();
		Fit();

		Reposition();

#if 0
		// Subscribe to the parent window's visibility and iconise events to avoid 
		// the popup from lingering around long after the tree is gone (#5095)
		auto* parentWindow = wxGetTopLevelParent(parent);

		if (parentWindow != nullptr)
		{
			parentWindow->Bind(wxEVT_SHOW, &SearchPopupWindow::_onParentVisibilityChanged, this);
			parentWindow->Bind(wxEVT_ICONIZE, &SearchPopupWindow::_onParentMinimized, this);

			// Detect when the parent window is losing focus (e.g. by alt-tabbing)
			parentWindow->Bind(wxEVT_ACTIVATE, &SearchPopupWindow::_onParentActivate, this);

			// Detect parent window movements to reposition ourselves
			parentWindow->Bind(wxEVT_MOVE, &SearchPopupWindow::_onParentMoved, this);
		}
#endif
	}

	virtual ~ModifierHintPopup()
	{
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
		wxPoint popupPos = GetParent()->GetScreenPosition() + wxSize(0, GetParent()->GetSize().y - GetSize().y);
		Position(popupPos, wxSize(0, 0));
	}
#if 0
	void _onIdleClose(wxIdleEvent& ev)
	{
		_owner.Close();
		ev.Skip();
	}

	void _onParentActivate(wxActivateEvent& ev)
	{
		if (!ev.GetActive())
		{
			_owner.Close();
		}
	}

	void _onParentMoved(wxMoveEvent&)
	{
		Reposition();
	}

	void _onParentMinimized(wxIconizeEvent&)
	{
		// Close any searches when the parent window is minimized
		_owner.Close();
	}

	void _onParentVisibilityChanged(wxShowEvent& ev)
	{
		if (!ev.IsShown())
		{
			// Close any searches when the parent window is hidden
			_owner.Close();
		}
	}
#endif
};

}

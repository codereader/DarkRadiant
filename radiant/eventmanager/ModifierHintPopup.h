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

        _text = new wxStaticText(this, wxID_ANY, "");
        GetSizer()->Add(_text, 1, wxALL, 3);
        SetText("");
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
};

}

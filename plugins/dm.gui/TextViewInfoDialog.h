#pragma once

#include "wxutil/dialog/DialogBase.h"

#include <wx/textctrl.h>
#include <wx/sizer.h>

namespace ui
{

///////////////////////////// TextViewInfoDialog:
// Small Info-Dialog showing text in a scrolled, non-editable textview and an ok button.
class TextViewInfoDialog :
	public wxutil::DialogBase
{
private:
	wxTextCtrl* _text;

public:
	TextViewInfoDialog(const std::string& title, const std::string& text,
					   wxWindow* parent = NULL,
					   int win_width = 650, int win_height = 500) :
		DialogBase(title),
		_text(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, 
			wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP))
	{
		SetSize(win_width, win_height);

		// Add a vbox for the dialog elements
		SetSizer(new wxBoxSizer(wxVERTICAL));

		// Add a vbox for the dialog elements
		wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
		GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

		vbox->Add(_text, 1, wxEXPAND | wxBOTTOM, 6);
		vbox->Add(CreateStdDialogButtonSizer(wxOK), 0, wxALIGN_RIGHT);

		_text->SetValue(text);

		CenterOnParent();
	}

	static void Show(const std::string& title, const std::string& text, wxWindow* parent = NULL)
	{
		TextViewInfoDialog* dialog = new TextViewInfoDialog(title, text, parent);

		dialog->ShowModal();
		dialog->Destroy();
	}
};

} // namespace

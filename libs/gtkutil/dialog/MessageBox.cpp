#include "MessageBox.h"

#include "imainframe.h"
#include "itextstream.h"
#include "i18n.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/frame.h>

namespace wxutil
{

Messagebox::Messagebox(const std::string& title, const std::string& text,
					   ui::IDialog::MessageType type, wxWindow* parent) :
	_dialog(new wxMessageDialog(parent != NULL ? parent : GlobalMainFrame().getWxTopLevelWindow(), 
			text, title, getDialogStyle(type)))
{
	if (type == ui::IDialog::MESSAGE_SAVECONFIRMATION)
	{
		_dialog->SetYesNoLabels(_("Save"), _("Close without saving"));
	}
}

Messagebox::~Messagebox()
{
	_dialog->Destroy();
}

void Messagebox::setTitle(const std::string& title)
{
	_dialog->SetTitle(title);
}

ui::IDialog::Result Messagebox::run()
{
	int returnCode = _dialog->ShowModal();

	// Map the wx result codes to ui::IDialog namespace
	switch (returnCode)
	{
		case wxID_OK:		return ui::IDialog::RESULT_OK;
		case wxID_CANCEL:	return ui::IDialog::RESULT_CANCELLED;
		case wxID_YES:		return ui::IDialog::RESULT_YES;
		case wxID_NO:		return ui::IDialog::RESULT_NO;
		default:			return ui::IDialog::RESULT_CANCELLED;
	};
}

long Messagebox::getDialogStyle(ui::IDialog::MessageType type)
{
	long style = wxCENTER;

	switch (type)
	{
		case ui::IDialog::MESSAGE_CONFIRM:
			style |= wxOK | wxICON_INFORMATION | wxOK_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_ASK:
			style |= wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_WARNING:
			style |= wxOK | wxICON_WARNING | wxOK_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_ERROR:
			style |= wxOK | wxICON_ERROR | wxOK_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_YESNOCANCEL:
			style |= wxYES_NO | wxCANCEL | wxICON_QUESTION | wxYES_DEFAULT;
			break;
        case ui::IDialog::MESSAGE_SAVECONFIRMATION:
			style |= wxYES_NO | wxCANCEL | wxICON_WARNING | wxYES_DEFAULT;
			break;
	};

	return style;
}

ui::IDialog::Result Messagebox::Show(const std::string& title,
	const std::string& text, ui::IDialog::MessageType type, wxWindow* parent)
{
	Messagebox msg(title, text, type, parent);
	
	return msg.run();
}

void Messagebox::ShowError(const std::string& errorText, wxWindow* parent)
{
	Messagebox msg("Error", errorText, ui::IDialog::MESSAGE_ERROR, parent);
	msg.run();
}

void Messagebox::ShowFatalError(const std::string& errorText, wxWindow* parent)
{
	ShowError(errorText, parent);
	abort();
}

} // namespace

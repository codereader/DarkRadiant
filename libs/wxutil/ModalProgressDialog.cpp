#include "ModalProgressDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include <wx/frame.h>

namespace wxutil
{

ModalProgressDialog::ModalProgressDialog(const std::string& title, wxWindow* parent) :
    wxProgressDialog(title, "", 100, parent != nullptr ? parent : GlobalMainFrame().getWxTopLevelWindow(), 
                     wxPD_CAN_ABORT | wxPD_APP_MODAL | wxPD_AUTO_HIDE)
{}

void ModalProgressDialog::setText(const std::string& text)
{
	// If the aborted flag is set, throw an exception here
	if (WasCancelled())
	{
		throw OperationAbortedException(_("Operation cancelled by user"));
	}

	Pulse(text);
}

void ModalProgressDialog::setTextAndFraction(const std::string& text, double fraction)
{
	if (WasCancelled())
	{
		throw OperationAbortedException(_("Operation cancelled by user"));
	}

	if (fraction < 0) 
	{
		fraction = 0.0;
	}
	else if (fraction > 1.0)
	{
		fraction = 1.0;
	}

	int newValue = static_cast<int>(fraction * 100);

	Update(newValue, text);
}

} // namespace

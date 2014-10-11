#include "QuerySidesDialog.h"

#include "i18n.h"
#include "imainframe.h"

#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Enter Number of Sides");
}

QuerySidesDialog::QuerySidesDialog(int numSidesMin, int numSidesMax) :
	DialogBase(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow()),
	_entry(NULL),
	_numSides(-1),
	_numSidesMin(numSidesMin),
	_numSidesMax(numSidesMax)
{
	// Create all the widgets
	populateWindow();

	Fit();
	CentreOnScreen();
}

int QuerySidesDialog::QueryNumberOfSides(int numSidesMin, int numSidesMax)
{
	QuerySidesDialog* dialog = new QuerySidesDialog(numSidesMin, numSidesMax);

	// Enter main loop
	int numSides = (dialog->ShowModal() == wxID_OK) ? dialog->_entry->GetValue() : -1;

	dialog->Destroy();

	return numSides;
}

void QuerySidesDialog::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* entrySizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Number of sides: "));
	
	_entry = new wxSpinCtrl(this, wxID_ANY);
	_entry->SetMinSize(wxSize(55, -1));
	_entry->SetRange(_numSidesMin, _numSidesMax);
	_entry->SetValue(_numSidesMin);

	entrySizer->Add(label, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
	entrySizer->Add(_entry, 0);

	GetSizer()->Add(entrySizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK|wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
}

} // namespace ui

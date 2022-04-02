#include "QuerySidesDialog.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "selectionlib.h"
#include "command/ExecutionNotPossible.h"
#include "command/ExecutionFailure.h"

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
	_entry(nullptr),
	_numSides(-1),
	_numSidesMin(numSidesMin),
	_numSidesMax(numSidesMax)
{
	// Create all the widgets
	populateWindow();

	Fit();
	CentreOnScreen();
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

void QuerySidesDialog::Show(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 0)
	{
		// Display a modal error dialog
		throw cmd::ExecutionNotPossible(_("At least one brush must be selected for this operation."));
	}

	if (args.size() != 1)
	{
		rError() << "Usage: " << std::endl
			<< "QueryBrushSidesDialog " << static_cast<int>(brush::PrefabType::Prism) << " <numSides> --> prism " << std::endl
			<< "QueryBrushSidesDialog " << static_cast<int>(brush::PrefabType::Cone) << " <numSides> --> cone " << std::endl
			<< "QueryBrushSidesDialog " << static_cast<int>(brush::PrefabType::Sphere) << " <numSides> --> sphere " << std::endl;
		return;
	}

	int input = args[0].getInt();

	if (input >= static_cast<int>(brush::PrefabType::Cuboid) && input < static_cast<int>(brush::PrefabType::NumPrefabTypes))
	{
		// Boundary checks passed
		auto type = static_cast<brush::PrefabType>(input);

		int minSides = 3;
		int maxSides = static_cast<int>(brush::PRISM_MAX_SIDES);

		switch (type)
		{
		case brush::PrefabType::Cuboid:
			minSides = 4;
			maxSides = 4;
			return;

		case brush::PrefabType::Prism:
			minSides = static_cast<int>(brush::PRISM_MIN_SIDES);
			maxSides = static_cast<int>(brush::PRISM_MAX_SIDES);
			break;

		case brush::PrefabType::Cone:
			minSides = static_cast<int>(brush::CONE_MIN_SIDES);
			maxSides = static_cast<int>(brush::CONE_MAX_SIDES);
			break;

		case brush::PrefabType::Sphere:
			minSides = static_cast<int>(brush::SPHERE_MIN_SIDES);
			maxSides = static_cast<int>(brush::SPHERE_MAX_SIDES);
			break;

		default:
			throw cmd::ExecutionFailure(fmt::format(_("Unknown brush type ID: {0}"), static_cast<int>(type)));
		};

		auto* dialog = new QuerySidesDialog(minSides, maxSides);

		// Enter main loop
		int numSides = (dialog->ShowModal() == wxID_OK) ? dialog->_entry->GetValue() : -1;

		dialog->Destroy();

		if (numSides != -1)
		{
			GlobalCommandSystem().executeCommand("BrushMakePrefab", cmd::Argument(input), cmd::Argument(numSides));
		}
	}
	else
	{
		throw cmd::ExecutionFailure(fmt::format(_("Unknown brush type ID: {0}"), input));
	}
}

} // namespace ui

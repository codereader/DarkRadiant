#include "MissionLogicDialog.h"
#include "ObjectiveEntity.h"

#include "i18n.h"
#include "string/string.h"

#include <boost/format.hpp>

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace objectives
{

namespace
{

const char* const DIALOG_TITLE = N_("Edit Mission Logic");

const char* const STANDARD_LOGIC_DESCR =
	N_("This is the standard logic for all difficulty levels");

const char* const DIFFICULTY_LOGIC_DESCR =
	N_("These logics override the standard logic for the given difficulty level\n"
		"if the logic string is non-empty.");

}

// Main constructor
MissionLogicDialog::MissionLogicDialog(wxWindow* parent, ObjectiveEntity& objectiveEnt) :
	DialogBase(_(DIALOG_TITLE), parent),
	_objectiveEnt(objectiveEnt)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxALL, 12);

	// Create one logic editor for each difficulty level plus the default one
	createLogicEditors();

	// Overall VBox for labels and alignments
	wxStaticText* defaultLabel = new wxStaticText(this, wxID_ANY, _("Default Logic"));
	defaultLabel->SetFont(defaultLabel->GetFont().Bold());
	vbox->Add(defaultLabel, 0, wxBOTTOM, 12);

	// Default Logic
	wxBoxSizer* defaultVBox = new wxBoxSizer(wxVERTICAL);
	defaultVBox->Add(new wxStaticText(this, wxID_ANY, _(STANDARD_LOGIC_DESCR)), 0, wxBOTTOM, 6);
	defaultVBox->Add(_logicEditors[-1], 0, wxBOTTOM | wxEXPAND, 6);

	vbox->Add(defaultVBox, 0, wxLEFT | wxEXPAND, 12);

	// Now add all difficulty-specific editors
	vbox->Add(new wxStaticText(this, wxID_ANY, _("Difficulty-specific Logic")), 0, wxBOTTOM, 12);

	wxBoxSizer* diffVBox = new wxBoxSizer(wxVERTICAL);
	diffVBox->Add(new wxStaticText(this, wxID_ANY, _(DIFFICULTY_LOGIC_DESCR)), 0, wxBOTTOM, 6);

	// Iterate over all editors for levels that are greater or equal 0
	for (LogicEditorMap::iterator i = _logicEditors.lower_bound(0);
		 i != _logicEditors.end(); ++i)
	{
		std::string logicStr = (boost::format(_("Logic for Difficulty Level %d")) % i->first).str();

		wxStaticText* logicLabel = new wxStaticText(this, wxID_ANY, logicStr); 
		logicLabel->SetFont(logicLabel->GetFont().Bold());
		diffVBox->Add(logicLabel, 0, wxBOTTOM, 6);

		diffVBox->Add(i->second, 0, wxEXPAND | wxBOTTOM, 6);
	}

	vbox->Add(diffVBox, 0, wxLEFT | wxEXPAND, 12);

	// Populate the logic strings
	populateLogicEditors();

	Layout();
	Fit();
}

void MissionLogicDialog::createLogicEditors()
{
	// Create the default logic editor
	_logicEditors[-1] = new LogicEditor(this);

	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are (and what their names are)
	_logicEditors[0] = new LogicEditor(this);
	_logicEditors[1] = new LogicEditor(this);
	_logicEditors[2] = new LogicEditor(this);
}

void MissionLogicDialog::populateLogicEditors()
{
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are
	for (int i = -1; i <= 2; i++)
	{
		LogicPtr logic = _objectiveEnt.getMissionLogic(i);

		// FIXME: Hm, maybe it would be better to pass the Logic object itself to the editor?
		_logicEditors[i]->setSuccessLogicStr(logic->successLogic);
		_logicEditors[i]->setFailureLogicStr(logic->failureLogic);
	}
}

int MissionLogicDialog::ShowModal()
{
	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		save();
	}

	return returnCode;
}

void MissionLogicDialog::save()
{
	// TODO: Connect this plugin to the difficulty plugin (which can be optional)
	// to find out how many difficulty levels there are
	for (int i = -1; i <= 2; i++)
	{
		LogicPtr logic = _objectiveEnt.getMissionLogic(i);

		// FIXME: Hm, maybe it would be better to pass the Logic object itself to the editor?
		logic->successLogic = _logicEditors[i]->getSuccessLogicStr();
		logic->failureLogic = _logicEditors[i]->getFailureLogicStr();
	}
}

} // namespace objectives

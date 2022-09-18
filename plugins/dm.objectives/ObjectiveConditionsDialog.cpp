#include "ObjectiveConditionsDialog.h"

#include "ui/imainframe.h"
#include "i18n.h"
#include "itextstream.h"

#include "string/convert.h"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>

#include "wxutil/ChoiceHelper.h"
#include "fmt/format.h"

#include "ObjectiveEntity.h"

namespace objectives
{

namespace
{
	const char* const DIALOG_TITLE = N_("Edit Objective Conditions");

	const std::string RKEY_ROOT = "user/ui/objectivesEditor/conditionsDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

ObjectiveConditionsDialog::ObjectiveConditionsDialog(wxWindow* parent, ObjectiveEntity& objectiveEnt) :
	DialogBase(_(DIALOG_TITLE), parent),
	_objectiveEnt(objectiveEnt),
	_objectiveConditionList(new wxutil::TreeModel(_objConditionColumns, true)),
	_srcObjState(NULL),
	_type(NULL),
	_value(NULL),
	_targetObj(NULL),
	_updateActive(false)
{
	wxPanel* mainPanel = loadNamedPanel(this, "ObjCondDialogMainPanel");
    
	makeLabelBold(this, "ObjCondDialogTopLabel");
	makeLabelBold(this, "ObjCondDialogConditionLabel");
	makeLabelBold(this, "ObjCondDialogSentenceLabel");

	// OK and CANCEL actions
	findNamedObject<wxButton>(this, "ObjCondDialogCancelButton")->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(ObjectiveConditionsDialog::_onCancel), NULL, this);
	findNamedObject<wxButton>(this, "ObjCondDialogOkButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ObjectiveConditionsDialog::_onOK), NULL, this);

	// Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

	// Copy the objective conditions to our working set
	_objConditions = _objectiveEnt.getObjectiveConditions();

	setupConditionsPanel();
	setupConditionEditPanel();

	updateSentence();

	mainPanel->Layout();
	mainPanel->Fit();
	Fit();
}

void ObjectiveConditionsDialog::setupConditionsPanel()
{
	wxPanel* condPanel = findNamedObject<wxPanel>(this, "ObjCondDialogConditionViewPanel");

	// Tree view listing the conditions
	_conditionsView = wxutil::TreeView::CreateWithModel(condPanel, _objectiveConditionList.get(), wxDV_NO_HEADER);
	condPanel->GetSizer()->Add(_conditionsView, 1, wxEXPAND);

	_conditionsView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(ObjectiveConditionsDialog::_onConditionSelectionChanged), NULL, this);
	
	// Number column
	_conditionsView->AppendTextColumn("", _objConditionColumns.conditionNumber.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Description
	_conditionsView->AppendTextColumn("", _objConditionColumns.description.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	
    // Connect button signals
	wxButton* addButton = findNamedObject<wxButton>(this, "ObjCondDialogAddConditionButton");
	addButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectiveConditionsDialog::_onAddObjCondition), NULL, this);

    wxButton* delButton = findNamedObject<wxButton>(this, "ObjCondDialogDeleteConditionButton");
	delButton->Enable(false); // disabled at start
	delButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectiveConditionsDialog::_onDelObjCondition), NULL, this);
}

void ObjectiveConditionsDialog::setupConditionEditPanel()
{
	// Initially everything is insensitive
	findNamedObject<wxButton>(this, "ObjCondDialogDeleteConditionButton")->Enable(false);

	// Disable details controls
    findNamedObject<wxPanel>(this, "ObjCondDialogConditionEditPanel")->Enable(false);

	// Set ranges for spin buttons
	wxSpinCtrl* srcMission = findNamedObject<wxSpinCtrl>(this, "ObjCondDialogSourceMission");
	srcMission->SetRange(1, 99);
	srcMission->SetValue(1);
	srcMission->Connect(wxEVT_SPINCTRL, wxSpinEventHandler(ObjectiveConditionsDialog::_onSrcMissionChanged), NULL, this);

	wxSpinCtrl* srcObj = findNamedObject<wxSpinCtrl>(this, "ObjCondDialogSourceObjective");
	srcObj->SetRange(1, 999);
	srcObj->SetValue(1);
	srcObj->Connect(wxEVT_SPINCTRL, wxSpinEventHandler(ObjectiveConditionsDialog::_onSrcObjChanged), NULL, this); 

	// State dropdown
	_srcObjState = findNamedObject<wxChoice>(this, "ObjCondDialogSourceObjectiveState");

	// Populate the list of states. This must be done in order to match the
	// values in the enum, since the index will be used when writing to entity
	_srcObjState->Append(Objective::getStateText(Objective::INCOMPLETE), 
		new wxStringClientData(string::to_string(Objective::INCOMPLETE)));
	_srcObjState->Append(Objective::getStateText(Objective::COMPLETE),
		new wxStringClientData(string::to_string(Objective::COMPLETE)));
	_srcObjState->Append(Objective::getStateText(Objective::INVALID),
		new wxStringClientData(string::to_string(Objective::INVALID)));
	_srcObjState->Append(Objective::getStateText(Objective::FAILED),
		new wxStringClientData(string::to_string(Objective::FAILED)));

	_srcObjState->Connect(wxEVT_CHOICE, 
		wxCommandEventHandler(ObjectiveConditionsDialog::_onSrcStateChanged), NULL, this); 

	// Objectives dropdown
	_targetObj = findNamedObject<wxChoice>(this, "ObjCondDialogTargetObjective");

	// Populate the liststore
	_objectiveEnt.populateChoice(_targetObj);

	_targetObj->Connect(wxEVT_CHOICE, 
		wxCommandEventHandler(ObjectiveConditionsDialog::_onTargetObjChanged), NULL, this);

	_type = findNamedObject<wxChoice>(this, "ObjCondDialogAction");

	_type->Append(_("Change Objective State"),
		new wxStringClientData(string::to_string(0)));
	_type->Append(_("Change Visibility"),
		new wxStringClientData(string::to_string(1)));
	_type->Append(_("Change Mandatory Flag"),
		new wxStringClientData(string::to_string(2)));

	_type->Connect(wxEVT_CHOICE, 
		wxCommandEventHandler(ObjectiveConditionsDialog::_onTypeChanged), NULL, this);

	_value = findNamedObject<wxChoice>(this, "ObjCondDialogActionValue");

	// Will be populated later on
	_value->Connect(wxEVT_CHOICE, 
		wxCommandEventHandler(ObjectiveConditionsDialog::_onValueChanged), NULL, this);
}

ObjectiveCondition& ObjectiveConditionsDialog::getCurrentObjectiveCondition()
{
	wxutil::TreeModel::Row row(_curCondition, *_objectiveConditionList);
	int index = row[_objConditionColumns.conditionNumber].getInteger();

	return *_objConditions[index];
}

void ObjectiveConditionsDialog::loadValuesFromCondition()
{
	_updateActive = true;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Source mission number
	wxSpinCtrl* srcMission = findNamedObject<wxSpinCtrl>(this, "ObjCondDialogSourceMission");
	srcMission->SetValue(cond.sourceMission + 1); // +1 since user-visible values are 1-based

	// Source objective number
	wxSpinCtrl* srcObj = findNamedObject<wxSpinCtrl>(this, "ObjCondDialogSourceObjective");
	srcObj->SetValue(cond.sourceObjective + 1); // +1 since user-visible values are 1-based

	// Source objective state
	wxutil::ChoiceHelper::SelectItemByStoredId(_srcObjState, cond.sourceState);

	// Find objective in dropdown list (stored objective numbers are 1-based)
	wxutil::ChoiceHelper::SelectItemByStoredId(_targetObj, cond.targetObjective+1);

	// Set condition type and load possible value types
	wxutil::ChoiceHelper::SelectItemByStoredId(_type, cond.type);

	refreshPossibleValues();

	updateSentence();

	_updateActive = false;
}

void ObjectiveConditionsDialog::refreshPossibleValues()
{
	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Remove all items from the dropdown
	_value->Clear();

	// Load possible value selections based on the selected type
	switch (cond.type)
	{
	case ObjectiveCondition::CHANGE_STATE:
		_value->Append(fmt::format(_("Set state to {0}"), Objective::getStateText(Objective::INCOMPLETE)));
		_value->Append(fmt::format(_("Set state to {0}"), Objective::getStateText(Objective::COMPLETE)));
		_value->Append(fmt::format(_("Set state to {0}"), Objective::getStateText(Objective::INVALID)));
		_value->Append(fmt::format(_("Set state to {0}"), Objective::getStateText(Objective::FAILED)));

		if (cond.value >= Objective::NUM_STATES)
		{
			cond.value = Objective::FAILED;
		}

		_value->Select(cond.value);
		break;

	case ObjectiveCondition::CHANGE_VISIBILITY:
		_value->Append(_("Set Invisible"));
		_value->Append(_("Set Visible"));

		if (cond.value > 1)
		{
			cond.value = 1;
		}

		_value->Select(cond.value);
		break;

	case ObjectiveCondition::CHANGE_MANDATORY:
		_value->Append(_("Clear mandatory flag"));
		_value->Append(_("Set mandatory flag"));

		if (cond.value > 1)
		{
			cond.value = 1;
		}

		_value->Select(cond.value);
		break;

	default:
		rWarning() << "Unknown type encountered while refreshing condition edit panel." << std::endl;
		break;
	};
}

void ObjectiveConditionsDialog::_onConditionSelectionChanged(wxDataViewEvent& ev)
{
	wxButton* delObjCondButton = findNamedObject<wxButton>(this, "ObjCondDialogDeleteConditionButton");
    
	// Get the selection
	_curCondition = _conditionsView->GetSelection();

	if (_curCondition.IsOk()) 
    {
		delObjCondButton->Enable(true);

		loadValuesFromCondition();

		// Enable details controls
		findNamedObject<wxPanel>(this, "ObjCondDialogConditionEditPanel")->Enable(true);
	}
	else
    {
		// No selection, disable the delete button 
		delObjCondButton->Enable(false);

		// Disable details controls
        findNamedObject<wxPanel>(this, "ObjCondDialogConditionEditPanel")->Enable(false);
	}
}

void ObjectiveConditionsDialog::_onAddObjCondition(wxCommandEvent& ev)
{
	for (int i = 1; i < INT_MAX; ++i)
	{
		ObjectiveEntity::ConditionMap::iterator found = _objConditions.find(i);

		if (found == _objConditions.end())
		{
			// Create a new condition
			ObjectiveConditionPtr cond(new ObjectiveCondition);
			_objConditions[i] = cond;

			// Set some default values, such that it doesn't end up invalid
			cond->sourceMission = 0;
			cond->sourceObjective = 0;
			cond->sourceState = Objective::INCOMPLETE;
			cond->targetObjective = 0;
			cond->type = ObjectiveCondition::CHANGE_STATE;
			cond->value = 0;

			// Refresh the dialog
			populateWidgets();

			// Select the new condition
			wxDataViewItem foundItem = _objectiveConditionList->FindInteger(i, 
				_objConditionColumns.conditionNumber);

			if (foundItem.IsOk())
			{
				_conditionsView->Select(foundItem);
			}

			return;
		}
	}

	throw std::runtime_error("Ran out of free objective condition indices.");
}

void ObjectiveConditionsDialog::_onDelObjCondition(wxCommandEvent& ev)
{
	assert(_curCondition.IsOk());

	// Get the index of the current objective condition
	wxutil::TreeModel::Row row(_curCondition, *_objectiveConditionList);
	int index = row[_objConditionColumns.conditionNumber].getInteger();

	_objConditions.erase(index);

	// Repopulate the dialog
	populateWidgets();
}

void ObjectiveConditionsDialog::_onTypeChanged(wxCommandEvent& ev)
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	cond.type = static_cast<ObjectiveCondition::Type>(wxutil::ChoiceHelper::GetSelectionId(_type));

	// Inhibit _onValueChanged calls
	_updateActive = true;

	refreshPossibleValues();

	_updateActive = false;

	updateSentence();
}

void ObjectiveConditionsDialog::_onSrcMissionChanged(wxSpinEvent& ev)
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Subtract 1 from the source mission, we need 0-based values
	wxSpinCtrl* srcMission = findNamedObject<wxSpinCtrl>(this, "ObjCondDialogSourceMission");
	cond.sourceMission = srcMission->GetValue() - 1;

	updateSentence();
}

void ObjectiveConditionsDialog::_onSrcObjChanged(wxSpinEvent& ev)
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Subtract 1 from the source objective, we need 0-based values
	wxSpinCtrl* srcObj = findNamedObject<wxSpinCtrl>(this, "ObjCondDialogSourceObjective");
	cond.sourceObjective = srcObj->GetValue() - 1;

	updateSentence();
}

void ObjectiveConditionsDialog::_onSrcStateChanged(wxCommandEvent& ev)
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	int state = wxutil::ChoiceHelper::GetSelectionId(_srcObjState);

	assert(state >= Objective::INCOMPLETE && state < Objective::NUM_STATES);
	cond.sourceState = static_cast<Objective::State>(state);

	updateSentence();
}

void ObjectiveConditionsDialog::_onTargetObjChanged(wxCommandEvent& ev)
{
	if (_updateActive || !isConditionSelected()) return;

	int objNum = wxutil::ChoiceHelper::GetSelectionId(_targetObj);

	if (objNum == -1)
	{
		return;
	}

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	cond.targetObjective = objNum - 1; // reduce by one, liststore has 1-based numbers

	updateSentence();
}

void ObjectiveConditionsDialog::_onValueChanged(wxCommandEvent& ev)
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	cond.value = _value->GetSelection();

	updateSentence();
}

void ObjectiveConditionsDialog::clear()
{
	// Clear the list
	_objectiveConditionList->Clear();
}

int ObjectiveConditionsDialog::ShowModal()
{
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();

	int returnCode = DialogBase::ShowModal();

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	// Clear all data before hiding
	clear();

	return returnCode;
}

void ObjectiveConditionsDialog::populateWidgets()
{
	// Clear internal data first
	clear();

	for (ObjectiveEntity::ConditionMap::const_iterator i = _objConditions.begin();
		 i != _objConditions.end(); ++i)
	{
		wxutil::TreeModel::Row row = _objectiveConditionList->AddItem();

		row[_objConditionColumns.conditionNumber] = i->first;
		row[_objConditionColumns.description] = getDescription(*i->second);

		row.SendItemAdded();
	}
}

std::string ObjectiveConditionsDialog::getDescription(const ObjectiveCondition& cond)
{
	return fmt::format(_("Condition affecting objective {0:d}"), (cond.targetObjective+1));
}

void ObjectiveConditionsDialog::_onCancel(wxCommandEvent& ev)
{
	EndModal(wxID_CANCEL);
}

void ObjectiveConditionsDialog::save()
{
	_objectiveEnt.setObjectiveConditions(_objConditions);
}

void ObjectiveConditionsDialog::_onOK(wxCommandEvent& ev)
{
	save();
	EndModal(wxID_OK);
}

bool ObjectiveConditionsDialog::isConditionSelected()
{
	return _conditionsView->GetSelection().IsOk();
}

std::string ObjectiveConditionsDialog::getSentence(const ObjectiveCondition& cond)
{
	std::string str = "";

	if (cond.isValid())
	{
		str = fmt::format(_("If Objective {0} in Mission {1} is in state '{2}' do the following: "), 
			(cond.sourceObjective+1), (cond.sourceMission+1), Objective::getStateText(cond.sourceState));

		str += "\n";

		std::string actionStr = "";
		int objNum = cond.targetObjective + 1; // the user-visible objectives are numbered differently

		switch (cond.type)
		{
		case ObjectiveCondition::CHANGE_STATE:
			actionStr = fmt::format(_("Set State on Objective {0} to {1}"),
				objNum, Objective::getStateText(static_cast<Objective::State>(cond.value)));
			break;

		case ObjectiveCondition::CHANGE_VISIBILITY:
			if (cond.value != 0)
			{
				actionStr = fmt::format(_("Make Objective {0} visible"), objNum);
			}
			else
			{
				actionStr = fmt::format(_("Make Objective {0} invisible"), objNum);
			}
			break;

		case ObjectiveCondition::CHANGE_MANDATORY:
			if (cond.value != 0)
			{
				actionStr = fmt::format(_("Make Objective {0} mandatory"), objNum);
			}
			else
			{
				actionStr = fmt::format(_("Make Objective {0} not mandatory"), objNum);
			}
			break;
        default:
            break;
		};

		str += actionStr;
	}
	else
	{
		str = _("This condition is not valid or complete yet.");
	}

	return str;
}

void ObjectiveConditionsDialog::updateSentence()
{
	wxStaticText* label = findNamedObject<wxStaticText>(this, "ObjCondDialogSentence");

	if (isConditionSelected())
	{
		label->SetLabelMarkup(getSentence(getCurrentObjectiveCondition()));
	}
	else
	{
		label->SetLabelMarkup("");
	}

	wxPanel* mainPanel = findNamedObject<wxPanel>(this, "ObjCondDialogMainPanel");
    
	mainPanel->Layout();
	mainPanel->Fit();
	Fit();
}

} // namespace

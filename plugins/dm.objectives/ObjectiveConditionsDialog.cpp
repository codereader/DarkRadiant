#include "ObjectiveConditionsDialog.h"

#include "imainframe.h"
#include "iuimanager.h"
#include "i18n.h"
#include "itextstream.h"

#include "string/convert.h"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>

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
	_objectives(Gtk::ListStore::create(_objectiveColumns)),
	_targetObj(NULL),
	_updateActive(false)
{
	loadNamedPanel(this, "ObjCondDialogMainPanel");
    
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
}

void ObjectiveConditionsDialog::setupConditionsPanel()
{
	wxPanel* condPanel = findNamedObject<wxPanel>(this, "ObjCondDialogConditionViewPanel");

	// Tree view listing the conditions
	_conditionsView = wxutil::TreeView::CreateWithModel(condPanel, _objectiveConditionList, wxDV_NO_HEADER);
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

	// Create the state dropdown, Glade is from the last century and doesn't support GtkComboBoxText, hmpf
	_srcObjState = findNamedObject<wxChoice>(this, "ObjCondDialogSourceObjectiveState");;

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

	// Create the objectives dropdown, populate from objective entity
	placeholder = gladeWidget<Gtk::VBox>("TargetObjectivePlaceholder");

	// Populate the liststore
	_objectiveEnt.populateListStore(_objectives, _objectiveColumns);

	// Set up the dropdown
	_targetObj = Gtk::manage(new Gtk::ComboBox(Glib::RefPtr<Gtk::TreeModel>::cast_static(_objectives)));

	Gtk::CellRendererText* indexRenderer = Gtk::manage(new Gtk::CellRendererText);
	Gtk::CellRendererText* nameRenderer = Gtk::manage(new Gtk::CellRendererText);
	
	_targetObj->pack_start(*indexRenderer, false);
	_targetObj->pack_start(*nameRenderer, true);
	_targetObj->add_attribute(indexRenderer->property_text(), _objectiveColumns.objNumber);
	_targetObj->add_attribute(nameRenderer->property_text(), _objectiveColumns.description);
	nameRenderer->set_property("width-chars", 30);
	
	_targetObj->signal_changed().connect(sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onTargetObjChanged));

	placeholder->pack_start(*_targetObj);

	placeholder = gladeWidget<Gtk::VBox>("TypePlaceholder");

	_type = Gtk::manage(new Gtk::ComboBoxText);

	_type->append_text(_("Change Objective State"));	// 0
	_type->append_text(_("Change Visibility"));			// 1
	_type->append_text(_("Change Mandatory Flag"));		// 2

	_type->signal_changed().connect(sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onTypeChanged));

	placeholder->pack_start(*_type);

	placeholder = gladeWidget<Gtk::VBox>("ValuePlaceholder");

	_value = Gtk::manage(new Gtk::ComboBoxText);

	// Will be populated later on
	_value->signal_changed().connect(sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onValueChanged));
	
	placeholder->pack_start(*_value);
}

ObjectiveCondition& ObjectiveConditionsDialog::getCurrentObjectiveCondition()
{
	int index = (*_curCondition)[_objConditionColumns.conditionNumber];

	return *_objConditions[index];
}

void ObjectiveConditionsDialog::loadValuesFromCondition()
{
	_updateActive = true;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Source mission number
	Gtk::SpinButton* srcMission = gladeWidget<Gtk::SpinButton>("SourceMission");
	srcMission->set_value(cond.sourceMission + 1); // +1 since user-visible values are 1-based

	// Source objective number
	Gtk::SpinButton* srcObj = gladeWidget<Gtk::SpinButton>("SourceObjective");
	srcObj->set_value(cond.sourceObjective + 1); // +1 since user-visible values are 1-based

	// Source objective state
	_srcObjState->set_active(cond.sourceState);

	// Find objective in dropdown list (stored objective numbers are 1-based)
	gtkutil::TreeModel::SelectionFinder finder(cond.targetObjective+1, _objectiveColumns.objNumber.index());
	_objectives->foreach_iter(sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	if (finder.getIter())
	{
		_targetObj->set_active(finder.getIter());
	}

	// Set condition type and load possible value types
	switch (cond.type)
	{
	case ObjectiveCondition::CHANGE_STATE:
		_type->set_active(0);
		break;

	case ObjectiveCondition::CHANGE_VISIBILITY:
		_type->set_active(1);
		break;

	case ObjectiveCondition::CHANGE_MANDATORY:
		_type->set_active(2);
		break;

	default:
		rWarning() << "Unknown type encountered while refreshing condition edit panel." << std::endl;
		_type->set_active(0);
		break;
	};

	refreshPossibleValues();

	updateSentence();

	_updateActive = false;
}

void ObjectiveConditionsDialog::refreshPossibleValues()
{
	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Remove all items from the dropdown
	_value->clear_items();

	// Load possible value selections based on the selected type
	switch (cond.type)
	{
	case ObjectiveCondition::CHANGE_STATE:
		_value->append_text((boost::format(_("Set state to %s")) % Objective::getStateText(Objective::INCOMPLETE)).str());
		_value->append_text((boost::format(_("Set state to %s")) % Objective::getStateText(Objective::COMPLETE)).str());
		_value->append_text((boost::format(_("Set state to %s")) % Objective::getStateText(Objective::INVALID)).str());
		_value->append_text((boost::format(_("Set state to %s")) % Objective::getStateText(Objective::FAILED)).str());

		if (cond.value >= Objective::NUM_STATES)
		{
			cond.value = Objective::FAILED;
		}

		_value->set_active(cond.value);
		break;

	case ObjectiveCondition::CHANGE_VISIBILITY:
		_value->append_text(_("Set Invisible"));
		_value->append_text(_("Set Visible"));

		if (cond.value > 1)
		{
			cond.value = 1;
		}

		_value->set_active(cond.value);

		break;

	case ObjectiveCondition::CHANGE_MANDATORY:
		_value->append_text(_("Clear mandatory flag"));
		_value->append_text(_("Set mandatory flag"));

		if (cond.value > 1)
		{
			cond.value = 1;
		}

		_value->set_active(cond.value);

		break;

	default:
		rWarning() << "Unknown type encountered while refreshing condition edit panel." << std::endl;
		break;
	};
}

void ObjectiveConditionsDialog::_onConditionSelectionChanged()
{
	Gtk::Button* delObjCondButton = gladeWidget<Gtk::Button>("delObjCondButton");
    
	// Get the selection
    Gtk::TreeView* condView = gladeWidget<Gtk::TreeView>("conditionsTreeView");

	_curCondition = condView->get_selection()->get_selected();

	if (_curCondition) 
    {
		delObjCondButton->set_sensitive(true);

		loadValuesFromCondition();

		// Enable details controls
        gladeWidget<Gtk::Widget>("ConditionVBox")->set_sensitive(true);
	}
	else
    {
		// No selection, disable the delete button 
		delObjCondButton->set_sensitive(false);

		// Disable details controls
        gladeWidget<Gtk::Widget>("ConditionVBox")->set_sensitive(false);
	}
}

void ObjectiveConditionsDialog::_onAddObjCondition()
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
			gtkutil::TreeModel::SelectionFinder finder(i, _objConditionColumns.conditionNumber.index());
			_objectiveConditionList->foreach_iter(sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

			if (finder.getIter())
			{
				gladeWidget<Gtk::TreeView>("conditionsTreeView")->get_selection()->select(finder.getIter());
			}

			return;
		}
	}

	throw std::runtime_error("Ran out of free objective condition indices.");
}

void ObjectiveConditionsDialog::_onDelObjCondition()
{
	assert(_curCondition);

	// Get the index of the current objective condition
	int index = (*_curCondition)[_objConditionColumns.conditionNumber];

	_objConditions.erase(index);

	// Repopulate the dialog
	populateWidgets();
}

void ObjectiveConditionsDialog::_onTypeChanged()
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	cond.type = static_cast<ObjectiveCondition::Type>(_type->get_active_row_number());

	// Inhibit _onValueChanged calls
	_updateActive = true;

	refreshPossibleValues();

	_updateActive = false;

	updateSentence();
}

void ObjectiveConditionsDialog::_onSrcMissionChanged()
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Subtract 1 from the source mission, we need 0-based values
	cond.sourceMission = gladeWidget<Gtk::SpinButton>("SourceMission")->get_value_as_int() - 1;

	updateSentence();
}

void ObjectiveConditionsDialog::_onSrcObjChanged()
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Subtract 1 from the source objective, we need 0-based values
	cond.sourceObjective = gladeWidget<Gtk::SpinButton>("SourceObjective")->get_value_as_int() - 1;

	updateSentence();
}

void ObjectiveConditionsDialog::_onSrcStateChanged()
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	int selectedRow = _srcObjState->get_active_row_number();

	assert(selectedRow >= Objective::INCOMPLETE && selectedRow < Objective::NUM_STATES);
	cond.sourceState = static_cast<Objective::State>(selectedRow);

	updateSentence();
}

void ObjectiveConditionsDialog::_onTargetObjChanged()
{
	if (_updateActive || !isConditionSelected()) return;

	if (!_targetObj->get_active())
	{
		return;
	}

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	int objNum = (*_targetObj->get_active())[_objectiveColumns.objNumber];
	 
	cond.targetObjective = objNum - 1; // reduce by one, liststore has 1-based numbers

	updateSentence();
}

void ObjectiveConditionsDialog::_onValueChanged()
{
	if (_updateActive || !isConditionSelected()) return;

	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	cond.value = _value->get_active_row_number();

	updateSentence();
}

void ObjectiveConditionsDialog::clear()
{
	// Clear the list
	_objectiveConditionList->clear();
}

void ObjectiveConditionsDialog::_preHide()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	// Clear all data before hiding
	clear();
}

void ObjectiveConditionsDialog::populateWidgets()
{
	// Clear internal data first
	clear();

	for (ObjectiveEntity::ConditionMap::const_iterator i = _objConditions.begin();
		 i != _objConditions.end(); ++i)
	{
		Gtk::TreeModel::Row row = *_objectiveConditionList->append();

		row[_objConditionColumns.conditionNumber] = i->first;
		row[_objConditionColumns.description] = getDescription(*i->second);
	}
}

std::string ObjectiveConditionsDialog::getDescription(const ObjectiveCondition& cond)
{
	return (boost::format(_("Condition affecting objective %d")) % (cond.targetObjective+1)).str();
}

void ObjectiveConditionsDialog::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();
}

void ObjectiveConditionsDialog::_onCancel()
{
	destroy();
}

void ObjectiveConditionsDialog::save()
{
	_objectiveEnt.setObjectiveConditions(_objConditions);
}

void ObjectiveConditionsDialog::_onOK()
{
	save();
	destroy();
}

bool ObjectiveConditionsDialog::isConditionSelected()
{
	return gladeWidget<Gtk::TreeView>("conditionsTreeView")->get_selection()->get_selected();
}

std::string ObjectiveConditionsDialog::getSentence(const ObjectiveCondition& cond)
{
	std::string str = "";

	if (cond.isValid())
	{
		str = (boost::format(_("If Objective %d in Mission %d is in state '%s' do the following: ")) % 
			(cond.sourceObjective+1) % (cond.sourceMission+1) % Objective::getStateText(cond.sourceState)).str();

		str += "\n";

		std::string actionStr = "";
		int objNum = cond.targetObjective + 1; // the user-visible objectives are numbered differently

		switch (cond.type)
		{
		case ObjectiveCondition::CHANGE_STATE:
			actionStr = (boost::format(_("Set State on Objective %d to %s")) % 
				objNum % Objective::getStateText(static_cast<Objective::State>(cond.value))).str();
			break;

		case ObjectiveCondition::CHANGE_VISIBILITY:
			if (cond.value != 0)
			{
				actionStr = (boost::format(_("Make Objective %d visible")) % objNum).str();
			}
			else
			{
				actionStr = (boost::format(_("Make Objective %d invisible")) % objNum).str();
			}
			break;

		case ObjectiveCondition::CHANGE_MANDATORY:
			if (cond.value != 0)
			{
				actionStr = (boost::format(_("Make Objective %d mandatory")) % objNum).str();
			}
			else
			{
				actionStr = (boost::format(_("Make Objective %d not mandatory")) % objNum).str();
			}
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
	Gtk::Label* label = gladeWidget<Gtk::Label>("Sentence");

	if (isConditionSelected())
	{
		label->set_markup(getSentence(getCurrentObjectiveCondition()));
	}
	else
	{
		label->set_markup("");
	}
}

} // namespace

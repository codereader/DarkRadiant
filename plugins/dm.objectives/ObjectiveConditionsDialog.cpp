#include "ObjectiveConditionsDialog.h"

#include "imainframe.h"
#include "iuimanager.h"
#include "i18n.h"

#include "gtkutil/TextColumn.h"

#include <gtkmm/button.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/treeview.h>
#include <gtkmm/box.h>

#include "ObjectiveEntity.h"

namespace objectives
{

namespace
{
	const char* const DIALOG_TITLE = N_("Edit Objective Conditions");

	const std::string RKEY_ROOT = "user/ui/objectivesEditor/conditionsDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

ObjectiveConditionsDialog::ObjectiveConditionsDialog(const Glib::RefPtr<Gtk::Window>& parent, 
	ObjectiveEntity& objectiveEnt) :
	gtkutil::BlockingTransientWindow(
        _(DIALOG_TITLE), GlobalMainFrame().getTopLevelWindow()
    ),
    gtkutil::GladeWidgetHolder(
        GlobalUIManager().getGtkBuilderFromFile("ObjectiveConditionsDialog.glade")
    ),
	_objectiveEnt(objectiveEnt),
	_objectiveConditionList(Gtk::ListStore::create(_objConditionColumns)),
	_objectives(Gtk::ListStore::create(_objectiveColumns))
{
	// Window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	// Add vbox to dialog
    add(*getGladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != NULL);

	// OK and CANCEL actions
	getGladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onCancel)
    );
	getGladeWidget<Gtk::Button>("okButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onOK)
    );

	// Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

	// Copy the objective conditions to our working set
	_objConditions = _objectiveEnt.getObjectiveConditions();

	setupConditionsPanel();
	setupConditionEditPanel();	
}

void ObjectiveConditionsDialog::setupConditionsPanel()
{
	// Tree view listing the conditions
    Gtk::TreeView* conditionsList = getGladeWidget<Gtk::TreeView>("conditionsTreeView");
    conditionsList->set_model(_objectiveConditionList);
	conditionsList->set_headers_visible(false);

	conditionsList->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onConditionSelectionChanged)
    );
	
	// Number column
	conditionsList->append_column(*Gtk::manage(new gtkutil::TextColumn("", _objConditionColumns.conditionNumber)));

	// Description
	conditionsList->append_column(*Gtk::manage(new gtkutil::TextColumn("", _objConditionColumns.description)));
	
    // Connect button signals
    Gtk::Button* addButton = getGladeWidget<Gtk::Button>("addObjCondButton");
	addButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onAddObjCondition)
    );

    Gtk::Button* delButton = getGladeWidget<Gtk::Button>("delObjCondButton");
	delButton->set_sensitive(false); // disabled at start
	delButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectiveConditionsDialog::_onDelObjCondition)
    );
}

void ObjectiveConditionsDialog::setupConditionEditPanel()
{
	// Create the state dropdown, Glade is from the last century and doesn't support GtkComboBoxText, hmpf
	Gtk::VBox* placeholder = getGladeWidget<Gtk::VBox>("SourceStatePlaceholder");

	_srcObjState = Gtk::manage(new Gtk::ComboBoxText);

	// Populate the list of states. This must be done in order to match the
	// values in the enum, since the index will be used when writing to entity
	_srcObjState->append_text("INCOMPLETE");
	_srcObjState->append_text("COMPLETE");
	_srcObjState->append_text("FAILED");
	_srcObjState->append_text("INVALID");

	placeholder->pack_start(*_srcObjState);

	// Create the objectives dropdown, populate from objective entity
	placeholder = getGladeWidget<Gtk::VBox>("TargetObjectivePlaceholder");

	// Populate the liststore
	_objectiveEnt.populateListStore(_objectives, _objectiveColumns);

	// Set up the dropdown
	_targetObj = Gtk::manage(new Gtk::ComboBox(_objectives));

	Gtk::CellRendererText* indexRenderer = Gtk::manage(new Gtk::CellRendererText);
	Gtk::CellRendererText* nameRenderer = Gtk::manage(new Gtk::CellRendererText);
	
	_targetObj->pack_start(*indexRenderer, false);
	_targetObj->pack_start(*nameRenderer, true);
	_targetObj->add_attribute(indexRenderer->property_text(), _objectiveColumns.objNumber);
	_targetObj->add_attribute(nameRenderer->property_text(), _objectiveColumns.description);
	
	placeholder->pack_start(*_targetObj);
}

ObjectiveCondition& ObjectiveConditionsDialog::getCurrentObjectiveCondition()
{
	int index = (*_curCondition)[_objConditionColumns.conditionNumber];

	return *_objConditions[index];
}

void ObjectiveConditionsDialog::refreshConditionPanel()
{
	ObjectiveCondition& cond = getCurrentObjectiveCondition();

	// Source mission number
	Gtk::SpinButton* srcMission = getGladeWidget<Gtk::SpinButton>("SourceMission");
	srcMission->set_value(cond.sourceMission);

	// Source objective number
	Gtk::SpinButton* srcObj = getGladeWidget<Gtk::SpinButton>("SourceObjective");
	srcObj->set_value(cond.sourceObjective);

	// Source objective state
	_srcObjState->set_active(cond.sourceState);

	// Load objectives from objective entity into dropdown list

	// TODO: Load target objective selection

	// TODO: Set condition type

	// TODO: Load value options based on condition type into dropdown list

	// TODO: Load value
}

void ObjectiveConditionsDialog::_onConditionSelectionChanged()
{
	Gtk::Button* delObjCondButton = getGladeWidget<Gtk::Button>("delObjCondButton");
    
	// Get the selection
    Gtk::TreeView* condView = getGladeWidget<Gtk::TreeView>("conditionsTreeView");

	_curCondition = condView->get_selection()->get_selected();

	if (_curCondition) 
    {
		delObjCondButton->set_sensitive(true);

		refreshConditionPanel();

		// Enable details controls
        getGladeWidget<Gtk::Widget>("ConditionVBox")->set_sensitive(true);
	}
	else
    {
		// No selection, disable the delete button 
		delObjCondButton->set_sensitive(false);

		// Disable details controls
        getGladeWidget<Gtk::Widget>("ConditionVBox")->set_sensitive(false);
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
			_objConditions[i] = ObjectiveConditionPtr(new ObjectiveCondition);

			// TODO: Select the new condition

			// Refresh the dialog
			populateWidgets();

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
		row[_objConditionColumns.description] = "empty description";
	}
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

} // namespace

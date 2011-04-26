#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/WindowPosition.h"

#include <gtkmm/liststore.h>
#include <gtkmm/comboboxtext.h>

#include "ObjectiveCondition.h"
#include "ObjectiveEntity.h"

namespace objectives
{

/**
 * Dialog for editing objective conditions (for use in TDM campaigns).
 */
class ObjectiveConditionsDialog :
	public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
private:
	// The objective entity we're working on
	ObjectiveEntity& _objectiveEnt;

	// UI struct defining an objective condition list entry
	struct ObjectiveConditionListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ObjectiveConditionListColumns()
		{
			add(conditionNumber);
			add(description);
		}

		Gtk::TreeModelColumn<int> conditionNumber;
		Gtk::TreeModelColumn<Glib::ustring> description;
	};

	// List of target_addobjectives entities
	ObjectiveConditionListColumns _objConditionColumns;
	Glib::RefPtr<Gtk::ListStore> _objectiveConditionList;

	// Iterators for current objective condition
	Gtk::TreeModel::iterator _curCondition;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

	// The working set of conditions, will be written to the ObjEntity on save
	ObjectiveEntity::ConditionMap _objConditions;

	// Source objective state choice
	Gtk::ComboBoxText* _srcObjState;

	// The target objective choice field, complete with model
	ObjectivesListColumns _objectiveColumns;
	Glib::RefPtr<Gtk::ListStore> _objectives;

	Gtk::ComboBox* _targetObj;

public:

	// Constructor creates widgets
	ObjectiveConditionsDialog(const Glib::RefPtr<Gtk::Window>& parent, ObjectiveEntity& objectiveEnt);

private:
	// Widget construction helpers
	void setupConditionsPanel();
	void setupConditionEditPanel();

	// gtkmm callbacks
	void _onCancel();
	void _onOK();

	/*
	void _onStartActiveCellToggled(const Glib::ustring& path);*/
	void _onConditionSelectionChanged();
	void _onAddObjCondition();
	void _onDelObjCondition();

	// Populate the dialog widgets with appropriate state from the objective entity
	void populateWidgets();

	// Refresh the condition editing panel
	void refreshConditionPanel();

	// Return the currently-selected objective condition
	ObjectiveCondition& getCurrentObjectiveCondition();

	// Clears the internal containers
	void clear();

	virtual void _preHide();
	virtual void _preShow();

	// Save changes to objectives entity
	void save();
};

}

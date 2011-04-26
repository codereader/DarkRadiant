#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"

#include <gtkmm/liststore.h>

#include "ObjectiveCondition.h"

namespace objectives
{

class ObjectiveEntity;

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

	/*// List of actual objectives associated with the selected entity
	ObjectivesListColumns _objectiveColumns;
	Glib::RefPtr<Gtk::ListStore> _objectiveList;
	
	// Pointer to the worldspawn entity
	Entity* _worldSpawn;

	// Map of ObjectiveEntity objectives, indexed by the name of the world
	// entity
	ObjectiveEntityMap _entities;

	// Iterators for current entity and current objective
	ObjectiveEntityMap::iterator _curEntity;
	Gtk::TreeModel::iterator _curObjective;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

	// The list of objective eclasses (defined in the registry)
	std::vector<std::string> _objectiveEClasses;*/

public:

	// Constructor creates widgets
	ObjectiveConditionsDialog(const Glib::RefPtr<Gtk::Window>& parent, ObjectiveEntity& objectiveEnt);

	// Widget construction helpers
	/*void setupEntitiesPanel();
	void setupObjectivesPanel();
	Gtk::Widget& createObjectiveEditPanel();
	Gtk::Widget& createLogicPanel();
	Gtk::Widget& createButtons();
	*/
	// gtkmm callbacks
	void _onCancel();
	void _onOK();

	/*
	void _onStartActiveCellToggled(const Glib::ustring& path);
	void _onEntitySelectionChanged();
	void _onObjectiveSelectionChanged();
	void _onAddEntity();
	void _onDeleteEntity();
	void _onAddObjective();
	void _onEditObjective();
	void _onMoveUpObjective();
	void _onMoveDownObjective();
	void _onDeleteObjective();
	void _onClearObjectives();
	void _onEditLogic();

	// Populate the dialog widgets with appropriate state from the map
	void populateWidgets();
	void populateActiveAtStart();

	// Refresh the objectives list from the currently-selected ObjectiveEntity
	void refreshObjectivesList();

	// Return the currently-selected objective
	Objective& getCurrentObjective();

	// Clears the internal containers
	void clear();

	virtual void _preHide();
	virtual void _preShow();*/
};

}

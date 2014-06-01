#pragma once

#include "Objective.h"
#include "ObjectiveEntity.h"

#include "icommandsystem.h"
#include "gtkutil/dialog/DialogBase.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/XmlResourceBasedWidget.h"

#include "ObjectiveEntityFinder.h"

#include <map>
#include <string>
#include "gtkutil/TreeView.h"

/* FORWARD DECLS */
class Entity;

namespace objectives
{

/**
 * Dialog for adding and manipulating mission objectives in Dark Mod missions.
 */
class ObjectivesEditor :
	public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
	// List of target_addobjectives entities
	ObjectiveEntityListColumns _objEntityColumns;
	wxutil::TreeModel* _objectiveEntityList;
	wxutil::TreeView* _objectiveEntityView;

	// List of actual objectives associated with the selected entity
	ObjectivesListColumns _objectiveColumns;
	wxutil::TreeModel* _objectiveList;
	wxutil::TreeView* _objectiveView;
	
	// Pointer to the worldspawn entity
	Entity* _worldSpawn;

	// Map of ObjectiveEntity objectives, indexed by the name of the world
	// entity
	ObjectiveEntityMap _entities;

	// Iterators for current entity and current objective
	ObjectiveEntityMap::iterator _curEntity;
	wxDataViewItem _curObjective;

	// The list of objective eclasses (defined in the registry)
	std::vector<std::string> _objectiveEClasses;

	wxutil::WindowPosition _windowPosition;

private:

	// Constructor creates widgets
	ObjectivesEditor();

	// Widget construction helpers
	void setupEntitiesPanel();
	void setupObjectivesPanel();
	void createObjectiveEditPanel();
	void createLogicPanel();
	void createButtons();

	// callbacks
	void _onCancel(wxCommandEvent& ev);
	void _onOK(wxCommandEvent& ev);
	void _onStartActiveCellToggled(wxDataViewEvent& ev);
	void _onEntitySelectionChanged(wxDataViewEvent& ev);
	void _onObjectiveSelectionChanged(wxDataViewEvent& ev);
	void _onAddEntity(wxCommandEvent& ev);
	void _onDeleteEntity(wxCommandEvent& ev);
	void _onAddObjective(wxCommandEvent& ev);
	void _onEditObjective(wxCommandEvent& ev);
	void _onMoveUpObjective(wxCommandEvent& ev);
	void _onMoveDownObjective(wxCommandEvent& ev);
	void _onDeleteObjective(wxCommandEvent& ev);
	void _onClearObjectives(wxCommandEvent& ev);
	void _onEditLogic(wxCommandEvent& ev);
	void _onEditObjConditions(wxCommandEvent& ev);

	// Populate the dialog widgets with appropriate state from the map
	void populateWidgets();
	void populateActiveAtStart();

	// Refresh the objectives list from the currently-selected ObjectiveEntity
	void refreshObjectivesList();

	// Return the currently-selected objective
	Objective& getCurrentObjective();

	// Clears the internal containers
	void clear();

public:
	// Override DialogBase::ShowModal
	int ShowModal();

	/**
	 * Static method to display the Objectives Editor dialog.
	 */
	static void DisplayDialog(const cmd::ArgumentList& args);
};

}

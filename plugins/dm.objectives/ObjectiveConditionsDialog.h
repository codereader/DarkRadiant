#pragma once

#include "gtkutil/dialog/DialogBase.h"
#include "gtkutil/XmlResourceBasedWidget.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/TreeView.h"

#include "ObjectiveCondition.h"
#include "ObjectiveEntity.h"

class wxChoice;

namespace objectives
{

/**
 * Dialog for editing objective conditions (for use in TDM campaigns).
 */
class ObjectiveConditionsDialog :
	public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
	// The objective entity we're working on
	ObjectiveEntity& _objectiveEnt;

	// UI struct defining an objective condition list entry
	struct ObjectiveConditionListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ObjectiveConditionListColumns() :
			conditionNumber(add(wxutil::TreeModel::Column::Integer)),
			description(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column conditionNumber;
		wxutil::TreeModel::Column description;
	};

	// List of target_addobjectives entities
	ObjectiveConditionListColumns _objConditionColumns;
	wxutil::TreeModel* _objectiveConditionList;
	wxutil::TreeView* _conditionsView;

	// Iterators for current objective condition
	wxDataViewItem _curCondition;

	// The position/size memoriser
	wxutil::WindowPosition _windowPosition;

	// The working set of conditions, will be written to the ObjEntity on save
	ObjectiveEntity::ConditionMap _objConditions;

	// Source objective state choice
	wxChoice* _srcObjState;

	// The action type
	wxChoice* _type;

	// The action value
	wxChoice* _value;

	// The target objective choice field, complete with model
	wxChoice* _targetObj;

	// Flag to block callbacks
	bool _updateActive;

public:

	// Constructor creates widgets
	ObjectiveConditionsDialog(wxWindow* parent, ObjectiveEntity& objectiveEnt);

	// Override DialogBase
	int ShowModal();

private:
	// Widget construction helpers
	void setupConditionsPanel();
	void setupConditionEditPanel();

	void _onCancel(wxCommandEvent& ev);
	void _onOK(wxCommandEvent& ev);

	void _onConditionSelectionChanged(wxDataViewEvent& ev);
	void _onAddObjCondition(wxCommandEvent& ev);
	void _onDelObjCondition(wxCommandEvent& ev);

	void _onSrcMissionChanged(wxSpinEvent& ev);
	void _onSrcObjChanged(wxSpinEvent& ev);
	void _onSrcStateChanged(wxCommandEvent& ev);
	void _onTargetObjChanged(wxCommandEvent& ev);
	void _onTypeChanged(wxCommandEvent& ev);
	void _onValueChanged(wxCommandEvent& ev);

	void updateSentence();

	// returns true if there is a condition highlighted
	bool isConditionSelected();

	// Populate the dialog widgets with appropriate state from the objective entity
	void populateWidgets();

	// Refresh the condition editing panel from the selected condition
	void loadValuesFromCondition();

	void refreshPossibleValues();

	// Return the currently-selected objective condition
	ObjectiveCondition& getCurrentObjectiveCondition();

	// Helper method to get a brief description of the given condition
	static std::string getDescription(const ObjectiveCondition& cond);

	// Helper to get a human-readable sentence what the given condition is doing
	std::string getSentence(const ObjectiveCondition& cond);

	// Clears the internal containers
	void clear();

	// Save changes to objectives entity
	void save();
};

}

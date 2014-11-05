#pragma once

#include "wxutil/dialog/DialogBase.h"
#include <map>
#include "Filter.h"
#include "wxutil/TreeView.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{

/**
 * greebo: UI for editing a single filter (name and criteria)
 */
class FilterEditor :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	// The actual filter object to be edited
	Filter& _originalFilter;

	// The working copy of the Filter
	Filter _filter;

	std::map<int, wxWindow*> _widgets;

	// Treemodel definition
	struct ListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListColumns() :
			index(add(wxutil::TreeModel::Column::Integer)),
			type(add(wxutil::TreeModel::Column::Integer)),
			typeString(add(wxutil::TreeModel::Column::String)),
			entityKey(add(wxutil::TreeModel::Column::String)),
			regexMatch(add(wxutil::TreeModel::Column::String)),
			showHide(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column index;
		wxutil::TreeModel::Column type;
		wxutil::TreeModel::Column typeString;
		wxutil::TreeModel::Column entityKey;
		wxutil::TreeModel::Column regexMatch;
		wxutil::TreeModel::Column showHide;
	};

	ListColumns _columns;

	wxutil::TreeModel::Ptr _ruleStore;
	wxutil::TreeView* _ruleView;

	// Treemodel definition
	struct TypeStoreColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TypeStoreColumns() :
			type(add(wxutil::TreeModel::Column::Integer)),
			typeString(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column type;
		wxutil::TreeModel::Column typeString;
	};

	TypeStoreColumns _typeStoreColumns;
	wxutil::TreeModel::Ptr _typeStore;

	// Treemodel definition
	struct ActionStoreColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ActionStoreColumns() :
			boolean(add(wxutil::TreeModel::Column::Boolean)),
			action(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column boolean;
		wxutil::TreeModel::Column action;
	};

	ActionStoreColumns _actionStoreColumns;
	wxutil::TreeModel::Ptr _actionStore;

	int _selectedRule;

	bool _updateActive;
	bool _viewOnly;

public:
	// Constructor, pass the Filter object to be edited
	FilterEditor(Filter& filter, wxWindow* parent, bool viewOnly);

private:
	void populateWindow();

	void save();

	void update();
	void updateWidgetSensitivity();

	void createCriteriaPanel();

	void createTypeStore();
	void createActionStore();

	// Converts the given enum into a string "entityclass", "object"
	std::string getStringForType(const FilterRule::Type type);
	FilterRule::Type getTypeForString(const std::string& typeStr);

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);

	void onAddRule(wxCommandEvent& ev);
	void onMoveRuleUp(wxCommandEvent& ev);
	void onMoveRuleDown(wxCommandEvent& ev);
	void onDeleteRule(wxCommandEvent& ev);

	void onNameEdited(wxCommandEvent& ev);

	void onItemEdited(wxDataViewEvent& ev);
	void onRuleSelectionChanged(wxDataViewEvent& ev);
};

} // namespace

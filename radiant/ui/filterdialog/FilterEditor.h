#pragma once

#include "wxutil/dialog/DialogBase.h"
#include <map>
#include "Filter.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include <wx/dataview.h>

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

    // List of filter rules in order
	wxDataViewListCtrl* _ruleList = nullptr;

	int _selectedRule = -1;

	bool _updateActive = false;
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

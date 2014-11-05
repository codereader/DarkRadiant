#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"
#include <map>
#include "Filter.h"
#include "wxutil/TreeView.h"
#include "wxutil/XmlResourceBasedWidget.h"

class wxButton;

namespace ui
{

class FilterDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			name(add(wxutil::TreeModel::Column::String)),
			state(add(wxutil::TreeModel::Column::String)),
			readonly(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column state;
		wxutil::TreeModel::Column readonly;
	};

	TreeColumns _columns;

	wxutil::TreeModel::Ptr _filterStore;

	// The treeview listing all the filters
	wxutil::TreeView* _filterView;

	// Holds the name of the currently selected filter (or "" if none selected)
	std::string _selectedFilter;

	typedef std::map<int, wxButton*> ButtonMap;
	ButtonMap _buttons;

	// The current working set, indexed by name
	typedef std::map<std::string, FilterPtr> FilterMap;
	FilterMap _filters;

	// The deleted filters, the actual deletion happens in the save() method
	FilterMap _deletedFilters;

	// Private constructor
	FilterDialog();

public:
	/**
	 * greebo: Shows the dialog (command target)
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	// Saves filter settings and exits
	void save();

	// Reload filter settings and fill widges
	void update();

	// Loads the filters from the filtersystem (happens at dialog construction)
	void loadFilters();

	// This is called to create the widgets
	void populateWindow();
	void createFiltersPanel();

	// Update buttons
	void updateWidgetSensitivity();

	// callbacks
	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);

	void onAddFilter(wxCommandEvent& ev);
	void onEditFilter(wxCommandEvent& ev);
	void onViewFilter(wxCommandEvent& ev);
	void onDeleteFilter(wxCommandEvent& ev);

	void onFilterSelectionChanged(wxDataViewEvent& ev);
};

} // namespace ui

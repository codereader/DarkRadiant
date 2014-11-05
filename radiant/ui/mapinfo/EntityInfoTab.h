#pragma once

#include "map/EntityBreakdown.h"

#include <wx/panel.h>
#include "wxutil/TreeView.h"

namespace ui
{

class EntityInfoTab :
	public wxPanel
{
private:
	// The helper class counting the entities in the map
	map::EntityBreakdown _entityBreakdown;

	// Treemodel definition
	struct ListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListColumns() :
			eclass(add(wxutil::TreeModel::Column::String)),
			count(add(wxutil::TreeModel::Column::Integer))
		{}

		wxutil::TreeModel::Column eclass;
		wxutil::TreeModel::Column count;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	wxutil::TreeModel::Ptr _listStore;
	wxutil::TreeView* _treeView;

public:
	// Constructor
	EntityInfoTab(wxWindow* parent);

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to create the widgets
	void populateTab();

}; // class EntityInfoTab

} // namespace ui

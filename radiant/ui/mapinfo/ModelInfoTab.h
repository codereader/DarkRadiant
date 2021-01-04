#pragma once

#include "scene/ModelBreakdown.h"

#include "wxutil/menu/PopupMenu.h"
#include <wx/panel.h>
#include "wxutil/dataview/TreeView.h"

namespace ui
{

class ModelInfoTab :
	public wxPanel
{
private:
	// The helper class counting the models in the map
	scene::ModelBreakdown _modelBreakdown;

	// Treemodel definition
	struct ListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListColumns() :
			model(add(wxutil::TreeModel::Column::String)),
			modelcount(add(wxutil::TreeModel::Column::Integer)),
			polycount(add(wxutil::TreeModel::Column::Integer)),
			skincount(add(wxutil::TreeModel::Column::Integer))
		{}

		wxutil::TreeModel::Column model;
		wxutil::TreeModel::Column modelcount;
		wxutil::TreeModel::Column polycount;
		wxutil::TreeModel::Column skincount;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	wxutil::TreeModel::Ptr _listStore;
	wxutil::TreeView* _treeView;

    wxutil::PopupMenuPtr _popupMenu;

public:
	// Constructor
	ModelInfoTab(wxWindow* parent);

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to create the widgets
	void populateTab();

    void _onSelectItems(bool select);
    bool _testSelectItems();
    void _onContextMenu(wxDataViewEvent& ev);

}; // class ModelInfoTab

} // namespace ui

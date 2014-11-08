#pragma once

#include "map/ShaderBreakdown.h"

#include "wxutil/menu/PopupMenu.h"
#include "wxutil/TreeView.h"
#include <wx/panel.h>

namespace ui
{

class ShaderInfoTab :
	public wxPanel
{
private:
	// The helper class counting the shaders in the map
	map::ShaderBreakdown _shaderBreakdown;

	// Treemodel definition
	struct ListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListColumns() :
			shader(add(wxutil::TreeModel::Column::String)),
			faceCount(add(wxutil::TreeModel::Column::Integer)),
			patchCount(add(wxutil::TreeModel::Column::Integer))
		{}

		wxutil::TreeModel::Column shader;
		wxutil::TreeModel::Column faceCount;
		wxutil::TreeModel::Column patchCount;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	wxutil::TreeModel::Ptr _listStore;
	wxutil::TreeView* _treeView;

	// Context menu
	wxutil::PopupMenuPtr _popupMenu;

public:
	// Constructor
	ShaderInfoTab(wxWindow* parent);

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to setup the widgets
	void construct();

	void _onSelectItems(bool select);
	bool _testSelectItems();
	void _onContextMenu(wxDataViewEvent& ev);

}; // class ShaderInfoTab

} // namespace ui

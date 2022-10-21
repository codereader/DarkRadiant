#pragma once

#include "iselection.h"
#include "GraphTreeModel.h"
#include <set>
#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"

namespace wxutil
{
	class TreeView;
}

class wxCheckBox;

namespace ui
{

class EntityList :
	public wxutil::DockablePanel,
    public selection::SelectionSystem::Observer
{
private:
	// The GraphTreeModel instance
	GraphTreeModel _treeModel;

	bool _callbackActive;

	wxutil::TreeView* _treeView;

	wxCheckBox* _focusSelected;
	wxCheckBox* _visibleOnly;

	sigc::connection _filtersConfigChangedConn;

	struct DataViewItemLess
	{
		bool operator() (const wxDataViewItem& a, const wxDataViewItem& b) const
		{
			return a.GetID() < b.GetID();
		}
	};

	std::set<wxDataViewItem, DataViewItemLess> _selection;

public:
	EntityList(wxWindow* parent);
    ~EntityList() override;

private:
	/** greebo: Creates the widgets
	 */
	void populateWindow();

	/** greebo: Updates the treeview contents
	 */
	void update();

    // Repopulate the entire treestore from the scenegraph
    void refreshTreeModel();

	/** 
	 * greebo: SelectionSystem::Observer implementation.
	 * Gets notified as soon as the selection is changed.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

	// Called by the graph tree model
	void onTreeViewSelection(const wxDataViewItem& item, bool selected);

	void onFilterConfigChanged();

	void onRowExpand(wxDataViewEvent& ev);

	// Called when the user is updating the treeview selection
	void onSelection(wxDataViewEvent& ev);
	void onVisibleOnlyToggle(wxCommandEvent& ev);

	void expandRootNode();
};

} // namespace

#pragma once

#include "ifilter.h"
#include "iselection.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "imodule.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/window/TransientWindow.h"
#include "GraphTreeModel.h"
#include <set>
#include <sigc++/connection.h>

namespace wxutil
{
	class TreeView;
}

class wxCheckBox;

namespace ui
{

class EntityList;
typedef std::shared_ptr<EntityList> EntityListPtr;

class EntityList :
	public wxutil::TransientWindow,
	public SelectionSystem::Observer
{
private:
	// The GraphTreeModel instance
	GraphTreeModel _treeModel;

	bool _callbackActive;

	wxutil::TreeView* _treeView;

	wxCheckBox* _focusSelected;
	wxCheckBox* _visibleOnly;

	sigc::connection _filtersChangedConnection;

	struct DataViewItemLess
	{
		bool operator() (const wxDataViewItem& a, const wxDataViewItem& b) const
		{
			return a.GetID() < b.GetID();
		}
	};

	std::set<wxDataViewItem, DataViewItemLess> _selection;

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static EntityListPtr& InstancePtr();

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
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	// Called by the graph tree model
	void onTreeViewSelection(const wxDataViewItem& item, bool selected);

	void filtersChanged();

	void onRowExpand(wxDataViewEvent& ev);

	// Called when the user is updating the treeview selection
	void onSelection(wxDataViewEvent& ev);
	void onVisibleOnlyToggle(wxCommandEvent& ev);

	void expandRootNode();

	// (private) Constructor, creates all the widgets
	EntityList();

	void _preHide();
	void _preShow();

public:
    ~EntityList();
    
	/** greebo: Shuts down this dialog, safely disconnects it
	 * 			from the EventManager and the SelectionSystem.
	 * 			Saves the window information to the Registry.
	 */
	void onRadiantShutdown();

	/** greebo: Toggles the window (command target).
	 */
	static void toggle(const cmd::ArgumentList& args);

	/** greebo: Contains the static instance. Use this
	 * 			to access the other members
	 */
	static EntityList& Instance();
};

} // namespace ui

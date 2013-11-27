#pragma once

#include "ifilter.h"
#include "iselection.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "imodule.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "GraphTreeModel.h"

namespace Gtk
{
	class TreeView;
	class CheckButton;
}

namespace ui
{

class EntityList;
typedef boost::shared_ptr<EntityList> EntityListPtr;

class EntityList :
	public gtkutil::PersistentTransientWindow,
	public SelectionSystem::Observer,
    private gtkutil::GladeWidgetHolder
{
private:
	// The GraphTreeModel instance
	GraphTreeModel _treeModel;

	gtkutil::WindowPosition _windowPosition;

	bool _callbackActive;

	sigc::connection _filtersChangedConnection;

private:
    Gtk::TreeView* treeView();
    Gtk::CheckButton* visibleOnly();

	// This is where the static shared_ptr of the singleton instance is held.
	static EntityListPtr& InstancePtr();

	// TransientWindow callbacks
	virtual void _preHide();
	virtual void _preShow();

	/** greebo: Creates the widgets
	 */
	void populateWindow();

	/** greebo: Updates the treeview contents
	 */
	void update();

	/** 
	 * greebo: SelectionSystem::Observer implementation.
	 * Gets notified as soon as the selection is changed.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	void filtersChanged();

	void onRowExpand(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path);
	bool onSelection(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool path_currently_selected);
	void onVisibleOnlyToggle();

	void expandRootNode();

	// (private) Constructor, creates all the widgets
	EntityList();

public:
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

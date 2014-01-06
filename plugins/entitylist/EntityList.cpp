#include "EntityList.h"

#include "ieventmanager.h"
#include "imainframe.h"
#include "iuimanager.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/TextColumn.h"
#include "registry/bind.h"
#include "entitylib.h"
#include "iselectable.h"
#include "icamera.h"
#include "i18n.h"

#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/treeselection.h>

namespace ui {

	namespace
	{
		const char* const WINDOW_TITLE = N_("Entity List");
		const std::string RKEY_ROOT = "user/ui/entityList/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

		const std::string RKEY_ENTITYLIST_FOCUS_SELECTION = RKEY_ROOT + "focusSelection";
		const std::string RKEY_ENTITYLIST_VISIBLE_ONLY = RKEY_ROOT + "visibleNodesOnly";
	}

EntityList::EntityList() :
	gtkutil::PersistentTransientWindow(
        _(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow(), true
    ),
    gtkutil::GladeWidgetHolder("EntityList.glade"),
	_callbackActive(false)
{
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create all the widgets and pack them into the window
	populateWindow();

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(this);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

Gtk::CheckButton* EntityList::visibleOnly()
{
    return gladeWidget<Gtk::CheckButton>("visibleOnly");
}

Gtk::TreeView* EntityList::treeView()
{
    return gladeWidget<Gtk::TreeView>("treeView");
}

void EntityList::populateWindow()
{
    add(*gladeWidget<Gtk::Widget>("main"));

	// Configure the treeview
    treeView()->set_model(_treeModel.getModel());

	Gtk::TreeViewColumn* column = Gtk::manage(
        new gtkutil::TextColumn(_("Name"), _treeModel.getColumns().name)
    );
	column->pack_start(*Gtk::manage(new Gtk::CellRendererText), true);

	Glib::RefPtr<Gtk::TreeSelection> sel = treeView()->get_selection();
	sel->set_mode(Gtk::SELECTION_MULTIPLE);
	sel->set_select_function(sigc::mem_fun(*this, &EntityList::onSelection));

	treeView()->signal_row_expanded().connect(sigc::mem_fun(*this, &EntityList::onRowExpand));

	treeView()->append_column(*column);
	column->set_sort_column(_treeModel.getColumns().name);
	column->clicked();

	// Update the toggle item status according to the registry
    registry::bindPropertyToKey(
        gladeWidget<Gtk::CheckButton>("focusSelected")->property_active(),
        RKEY_ENTITYLIST_FOCUS_SELECTION
    );
    registry::bindPropertyToKey(visibleOnly()->property_active(),
                                RKEY_ENTITYLIST_VISIBLE_ONLY);

	_treeModel.setConsiderVisibleNodesOnly(visibleOnly()->get_active());

	// Connect the toggle buttons' "toggled" signal
	visibleOnly()->signal_toggled().connect(
        sigc::mem_fun(*this, &EntityList::onVisibleOnlyToggle)
    );
}

void EntityList::update()
{
	// Disable callbacks and traverse the treemodel
	_callbackActive = true;

	// Traverse the entire tree, updating the selection
	_treeModel.updateSelectionStatus(treeView()->get_selection());

	_callbackActive = false;
}

// Gets notified upon selection change
void EntityList::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	if (_callbackActive || !is_visible() || isComponent)
	{
		// Don't update if not shown or already updating, also ignore components
		return;
	}

	_callbackActive = true;

	_treeModel.updateSelectionStatus(treeView()->get_selection(), node);

	_callbackActive = false;
}

void EntityList::filtersChanged()
{
    // Only react to filter changes if we display visible nodes only otherwise
    // we don't care
	if (visibleOnly()->get_active())
	{
		// When filter are changed possibly any node changed its visibility,
		// refresh the whole tree
		_treeModel.refresh();
		expandRootNode();
	}
}

// Pre-hide callback
void EntityList::_preHide()
{
	_treeModel.disconnectFromSceneGraph();

	// Disconnect from the filters-changed signal
	_filtersChangedConnection.disconnect();

	// De-register self from the SelectionSystem
	GlobalSelectionSystem().removeObserver(this);

	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void EntityList::_preShow()
{
	// Observe the scenegraph
	_treeModel.connectToSceneGraph();

	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);

	// Get notified when filters are changing
	_filtersChangedConnection = GlobalFilterSystem().filtersChangedSignal().connect(
        sigc::mem_fun(Instance(), &EntityList::filtersChanged)
    );

	// Restore the position
	_windowPosition.applyPosition();

	_callbackActive = true;

	// Repopulate the model before showing the dialog
	_treeModel.refresh();

	_callbackActive = false;

	// Update the widgets
	update();

	expandRootNode();
}

void EntityList::toggle(const cmd::ArgumentList& args)
{
	Instance().toggleVisibility();
}

void EntityList::onRadiantShutdown()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	// De-register self from the SelectionSystem
	GlobalEventManager().disconnectDialogWindow(this);

	// Destroy the transient window
	destroy();

	// Destroy the singleton
	InstancePtr().reset();
}

EntityListPtr& EntityList::InstancePtr()
{
	static EntityListPtr _instancePtr;
	return _instancePtr;
}

EntityList& EntityList::Instance()
{
	if (InstancePtr() == NULL)
	{
		// Not yet instantiated, do it now
		InstancePtr() = EntityListPtr(new EntityList);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*InstancePtr(), &EntityList::onRadiantShutdown)
        );
	}

	return *InstancePtr();
}

void EntityList::onRowExpand(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path)
{
	if (_callbackActive) return; // avoid loops

	// greebo: This is a possible optimisation point. Don't update the entire tree,
	// but only the expanded subtree.
	update();
}

void EntityList::onVisibleOnlyToggle()
{
	// Update the whole tree
	_treeModel.setConsiderVisibleNodesOnly(visibleOnly()->get_active());
	_treeModel.refresh();

	expandRootNode();
}

void EntityList::expandRootNode()
{
	GraphTreeNodePtr rootNode = _treeModel.find(GlobalSceneGraph().root());
	treeView()->expand_row(Gtk::TreeModel::Path(rootNode->getIter()), false);
}

bool EntityList::onSelection(const Glib::RefPtr<Gtk::TreeModel>& model,
							 const Gtk::TreeModel::Path& path,
							 bool path_currently_selected)
{
	if (_callbackActive) return true; // avoid loops

	Gtk::TreeModel::iterator iter = model->get_iter(path);

	// Load the instance pointer from the columns
	scene::INode* node = (*iter)[_treeModel.getColumns().node];

	Selectable* selectable = dynamic_cast<Selectable*>(node);

	if (selectable != NULL)
	{
		// We've found a selectable instance

		// Disable update to avoid loopbacks
		_callbackActive = true;

		// Select the instance
		selectable->setSelected(path_currently_selected == false);

		if (gladeWidget<Gtk::CheckButton>("focusSelected")->get_active())
		{
			const AABB& aabb = node->worldAABB();
			Vector3 origin(aabb.origin);

			// Move the camera a bit off the AABB origin
			origin += Vector3(-50, 0, 50);

			// Rotate the camera a bit towards the "ground"
			Vector3 angles(0, 0, 0);
			angles[CAMERA_PITCH] = -30;

			GlobalCameraView().focusCamera(origin, angles);
		}

		// Now reactivate the callbacks
		_callbackActive = false;

		return true; // don't propagate
	}

	return false;
}

} // namespace ui

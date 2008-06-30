#include "EntityList.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "nameable.h"
#include <gtk/gtk.h>
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "entitylib.h"
#include "scenelib.h"
#include "icamera.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Entity List";
		const std::string RKEY_ROOT = "user/ui/entityList/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

		const std::string RKEY_ENTITYLIST_FOCUS_SELECTION = RKEY_ROOT + "focusSelection";
	}

EntityList::EntityList() : 
	gtkutil::PersistentTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow(), true),
	_callbackActive(false)
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

void EntityList::destroyInstance() {
	if (InstancePtr() != NULL) {
		// De-register self from the SelectionSystem
		GlobalSelectionSystem().removeObserver(InstancePtr().get());
	}

	InstancePtr() = EntityListPtr();
}

void EntityList::populateWindow() {
	// Create the treeview
	_treeView = GTK_TREE_VIEW(gtk_tree_view_new());
	gtk_tree_view_set_headers_visible(_treeView, FALSE);
	
	gtk_tree_view_set_model(_treeView, _treeModel);
	
	GtkTreeViewColumn* column = gtkutil::TextColumn("Name", GraphTreeModel::COL_NAME);
	gtk_tree_view_column_pack_start(column, gtk_cell_renderer_text_new(), TRUE);
	
	_selection = gtk_tree_view_get_selection(_treeView);
	gtk_tree_selection_set_mode(_selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_select_function(_selection, onSelection, this, 0);
	
	g_signal_connect(G_OBJECT(_treeView), "row-expanded", G_CALLBACK(onRowExpand), this);
	
	gtk_tree_view_append_column (_treeView, column);
	gtk_tree_view_column_set_sort_column_id(column, GraphTreeModel::COL_NAME);
	gtk_tree_view_column_clicked(column);

	// Create the toggle item
	_focusOnSelectedEntityToggle = gtk_check_button_new_with_label("Focus on the selected entity in the camera.");

	// Update the toggle item status according to the registry
	bool isActive = GlobalRegistry().get(RKEY_ENTITYLIST_FOCUS_SELECTION) == "1";
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_focusOnSelectedEntityToggle), isActive);

	// Connect the toggle button's "toggled" signal
	g_signal_connect(G_OBJECT(_focusOnSelectedEntityToggle), "toggled", G_CALLBACK(onFocusSelectionToggle), this);
	
	// Create a VBOX
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::ScrolledFrame(GTK_WIDGET(_treeView)), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _focusOnSelectedEntityToggle, FALSE, FALSE, 0);

	// Pack the VBOX into the window
	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);	
}

void EntityList::update() {
	// Disable callbacks and traverse the treemodel
	_callbackActive = true;
	
	// Traverse the entire tree, updating the selection
	_treeModel.updateSelectionStatus(_selection);
	
	_callbackActive = false;
}

// Gets notified upon selection change
void EntityList::selectionChanged(const scene::INodePtr& node, bool isComponent) {
	if (_callbackActive || !isVisible() || isComponent) {
		// Don't update if not shown or already updating, also ignore components
		return;
	}
	
	_callbackActive = true;
	
	_treeModel.updateSelectionStatus(_selection, node);
	
	_callbackActive = false;
}

void EntityList::toggleWindow() {
	if (isVisible()) {
		hide();
	}
	else {
		show();
	}
}

// Pre-hide callback
void EntityList::_preHide() {
	_treeModel.disconnectFromSceneGraph();

	// De-register self from the SelectionSystem
	GlobalSelectionSystem().removeObserver(this);

	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void EntityList::_preShow() {
	// Observe the scenegraph
	_treeModel.connectToSceneGraph();

	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);

	// Restore the position
	_windowPosition.applyPosition();
	
	_callbackActive = true;
	
	// Repopulate the model before showing the dialog
	_treeModel.refresh();
	
	_callbackActive = false;
	
	// Update the widgets
	update();
}

void EntityList::toggle() {
	Instance().toggleWindow();
}

void EntityList::onRadiantShutdown() {

	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));

	// Destroy the transient window
	destroy();
}

EntityListPtr& EntityList::InstancePtr() {
	static EntityListPtr _instancePtr;
	return _instancePtr;
}

EntityList& EntityList::Instance() {
	if (InstancePtr() == NULL) {
		// Not yet instantiated, do it now
		InstancePtr() = EntityListPtr(new EntityList);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(InstancePtr());
	}
	
	return *InstancePtr();
}

void EntityList::onRowExpand(GtkTreeView* view, GtkTreeIter* iter, GtkTreePath* path, EntityList* self) {
	if (self->_callbackActive) return; // avoid loops

	// greebo: This is a possible optimisation point. Don't update the entire tree,
	// but only the expanded subtree.
	self->update();
}

void EntityList::onFocusSelectionToggle(GtkToggleButton* togglebutton, EntityList* self)
{
	// Update the registry state in the registry
	bool active = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(self->_focusOnSelectedEntityToggle)) ? true : false;

	GlobalRegistry().set(RKEY_ENTITYLIST_FOCUS_SELECTION, active ? "1" : "0");
}

gboolean EntityList::onSelection(GtkTreeSelection* selection, 
								GtkTreeModel* model, 
								GtkTreePath* path, 
								gboolean path_currently_selected, 
								gpointer data)
{
	// Get a pointer to the class instance
	EntityList* self = reinterpret_cast<EntityList*>(data);

	if (self->_callbackActive) return TRUE; // avoid loops

	bool shouldFocus = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(self->_focusOnSelectedEntityToggle)) ? true : false;
	
	if (!shouldFocus) return TRUE;

	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	
	// Load the instance pointer from the columns
	scene::INode* node = reinterpret_cast<scene::Node*>(
		gtkutil::TreeModel::getPointer(model, &iter, GraphTreeModel::COL_NODE_POINTER)
	);
	
	Selectable* selectable = dynamic_cast<Selectable*>(node);

	if (selectable != NULL) {
		// We've found a selectable instance
		
		// Disable update to avoid loopbacks
		self->_callbackActive = true;
		
		// Select the instance
		selectable->setSelected(path_currently_selected == FALSE);

		const AABB& aabb = node->worldAABB();
		Vector3 origin(aabb.origin);
		
		// Move the camera a bit off the AABB origin
		origin += Vector3(-50, 0, 50);

		// Rotate the camera a bit towards the "ground"
		Vector3 angles(0, 0, 0);
		angles[CAMERA_PITCH] = -30;

		GlobalCameraView().focusCamera(origin, angles);

		// Now reactivate the callbacks
		self->_callbackActive = false;
		
		return TRUE; // don't propagate
	}

	return FALSE;
}

} // namespace ui

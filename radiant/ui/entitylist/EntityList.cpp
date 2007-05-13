#include "EntityList.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "nameable.h"
#include <gtk/gtk.h>
#include "gtkutil/TransientWindow.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "entitylib.h"
#include "mainframe.h"
#include "map.h"
#include "scenelib.h"
#include "scenegraph.h"
#include "camera/Camera.h"
#include "treemodel.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Entity List";
		const std::string RKEY_ROOT = "user/ui/entityList/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		
		enum {
			NODE_COL,
			INSTANCE_COL,
			NUM_COLS
		};
	}

EntityList::EntityList() :
	_callbackActive(false)
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_dialog));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

	namespace {
		inline Nameable* Node_getNameable(scene::Node& node) {
			return dynamic_cast<Nameable*>(&node);
		}
		
		std::string getNodeName(scene::Node& node) {
			Nameable* nameable = Node_getNameable(node);
			return (nameable != NULL) ? nameable->name() : "node";
		}
		
		void cellDataFunc(GtkTreeViewColumn* column, GtkCellRenderer* renderer, 
						  GtkTreeModel* model, GtkTreeIter* iter, gpointer data)
		{
			// Load the pointers from the columns
			scene::Node* node = reinterpret_cast<scene::Node*>(
				gtkutil::TreeModel::getPointer(model, iter, NODE_COL));
			
			scene::Instance* instance = reinterpret_cast<scene::Instance*>(
				gtkutil::TreeModel::getPointer(model, iter, INSTANCE_COL));
			
			if (node != NULL) {
				gtk_cell_renderer_set_fixed_size(renderer, -1, -1);
				std::string name = getNodeName(*node);
				g_object_set(G_OBJECT(renderer), 
							 "text", name.c_str(), 
							 "visible", TRUE, 
							 NULL);
		
				GtkWidget* treeView = reinterpret_cast<GtkWidget*>(data);
		
				//globalOutputStream() << "rendering cell " << makeQuoted(name) << "\n";
				GtkStyle* style = gtk_widget_get_style(treeView);
				if (instance->childSelected()) {
					g_object_set(G_OBJECT(renderer), 
						"cell-background-gdk", &style->base[GTK_STATE_ACTIVE], NULL);
				}
				else {
					g_object_set(G_OBJECT(renderer), 
						"cell-background-gdk", &style->base[GTK_STATE_NORMAL], NULL);
				}
			}
			else {
				gtk_cell_renderer_set_fixed_size(renderer, -1, 0);
				g_object_set(G_OBJECT(renderer), "text", "", "visible", FALSE, NULL);
			}
		}
	}

void EntityList::populateWindow() {
	_treeView = GTK_TREE_VIEW(gtk_tree_view_new());
	gtk_tree_view_set_headers_visible(_treeView, FALSE);
	
	_treeModel = GTK_TREE_MODEL(scene_graph_get_tree_model());
	gtk_tree_view_set_model(_treeView, _treeModel);
	
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, cellDataFunc, _treeView, NULL);
	
	_selection = gtk_tree_view_get_selection(_treeView);
	gtk_tree_selection_set_mode(_selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_select_function(_selection, onSelection, this, 0);
	
	g_signal_connect(G_OBJECT(_treeView), "row-expanded", G_CALLBACK(onRowExpand), this);
	
	gtk_tree_view_append_column (_treeView, column);
	
	gtk_container_add(
		GTK_CONTAINER(_dialog), 
		gtkutil::ScrolledFrame(GTK_WIDGET(_treeView))
	);
}

gboolean EntityList::modelUpdater(GtkTreeModel* model, GtkTreePath* path, 
								  GtkTreeIter* iter, gpointer data)
{
	GtkTreeView* treeView = reinterpret_cast<GtkTreeView*>(data);
	
	// Load the pointers from the columns
	scene::Instance* instance = reinterpret_cast<scene::Instance*>(
		gtkutil::TreeModel::getPointer(model, iter, INSTANCE_COL));

	Selectable* selectable = Instance_getSelectable(*instance);
	
	if (selectable != NULL) {
		GtkTreeSelection* selection = gtk_tree_view_get_selection(treeView);
		if (selectable->isSelected()) {
			gtk_tree_selection_select_path(selection, path);
		}
		else {
			gtk_tree_selection_unselect_path(selection, path);
		}
	}

	return FALSE;
}

void EntityList::update() {
	// Disable callbacks and traverse the treemodel
	_callbackActive = true;
	gtk_tree_model_foreach(_treeModel, modelUpdater, _treeView);
	_callbackActive = false;
}

// Gets notified upon selection change
void EntityList::selectionChanged(scene::Instance& instance) {
	if (_callbackActive) return; // avoid loops
	
	update();
}

void EntityList::toggleWindow() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		// Save the window position, to make sure
		_windowPosition.readPosition();
		gtk_widget_hide_all(_dialog);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Update the widgets
		update();
		// Now show the dialog window again
		gtk_widget_show_all(_dialog);
	}
}

void EntityList::toggle() {
	Instance().toggleWindow();
}

void EntityList::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	gtk_widget_hide(_dialog);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_dialog));
}

EntityList& EntityList::Instance() {
	static EntityList _instance;
	return _instance;
}

void EntityList::onRowExpand(GtkTreeView* view, GtkTreeIter* iter, GtkTreePath* path, EntityList* self) {
	if (self->_callbackActive) return; // avoid loops

	self->update();
}

gboolean EntityList::onSelection(GtkTreeSelection *selection, 
								GtkTreeModel *model, 
								GtkTreePath *path, 
								gboolean path_currently_selected, 
								gpointer data)
{
	// Get a pointer to the class instance
	EntityList* self = reinterpret_cast<EntityList*>(data);
	
	if (self->_callbackActive) return TRUE; // avoid loops
	
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	
	// Load the pointers from the columns
	scene::Node* node = reinterpret_cast<scene::Node*>(
		gtkutil::TreeModel::getPointer(model, &iter, NODE_COL));
	
	scene::Instance* instance = reinterpret_cast<scene::Instance*>(
		gtkutil::TreeModel::getPointer(model, &iter, INSTANCE_COL));
	
	Selectable* selectable = Instance_getSelectable(*instance);

	if (node == NULL) {
		if (path_currently_selected != FALSE) {
			// Disable callbacks
			self->_callbackActive = true;
			
			// Deselect all
			GlobalSelectionSystem().setSelectedAll(false);
			
			// Now reactivate the callbacks
			self->_callbackActive = false;
		}
	}
	else if (selectable != NULL) {
		// We've found a selectable instance
		
		// Disable callbacks
		self->_callbackActive = true;
		
		// Select the instance
		selectable->setSelected(path_currently_selected == FALSE);

		// greebo: Grab the origin keyvalue from the entity and focus the view on it
		Entity* entity = Node_getEntity(*node);
		if (entity != NULL) {
			Vector3 entityOrigin(entity->getKeyValue("origin"));

			// Move the camera a bit off the entity origin
			entityOrigin += Vector3(-50, 0, 50);

			// Rotate the camera a bit towards the "ground"
			Vector3 angles(0, 0, 0);
			angles[CAMERA_PITCH] = -30;

			map::focusViews(entityOrigin, angles);
		}

		// Now reactivate the callbacks
		self->_callbackActive = false;
		
		return TRUE;
	}

	return FALSE;
}

gboolean EntityList::onDelete(GtkWidget* widget, GdkEvent* event, EntityList* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

} // namespace ui

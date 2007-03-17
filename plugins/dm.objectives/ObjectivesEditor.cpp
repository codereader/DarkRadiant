#include "ObjectivesEditor.h"
#include "ObjectiveEntityFinder.h"
#include "ObjectiveKeyExtractor.h"
#include "RandomOrigin.h"
#include "TargetList.h"

#include "iscenegraph.h"
#include "qerplugin.h"
#include "ieclass.h"
#include "ientity.h"

#include "scenelib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"

#include <gtk/gtk.h>

namespace objectives
{

/* CONSTANTS */
namespace {

	const char* DIALOG_TITLE = "Mission objectives"; 	
	
	// TODO: This should be in the .game file
	const char* OBJECTIVE_ENTITY_CLASS = "target_tdm_addobjectives";
	
}

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _objectiveEntityList(gtk_list_store_new(4, 
  										  G_TYPE_STRING, 		// display text
  										  G_TYPE_BOOLEAN,		// start active
  										  G_TYPE_POINTER,		// Entity*
  										  G_TYPE_POINTER)),		// scene::Node*
  _objectiveList(gtk_list_store_new(3, 
  								    G_TYPE_STRING,		// obj number 
  								    G_TYPE_STRING,		// obj description
  								    G_TYPE_POINTER))	// pointer to Objective
{
	// Window properties
	gtk_window_set_transient_for(
		GTK_WINDOW(_widget), GlobalRadiant().getMainWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), DIALOG_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    
    // Window size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gtk_window_set_default_size(GTK_WINDOW(_widget), 
								gint(gdk_screen_get_width(scr) * 0.5), 
								gint(gdk_screen_get_height(scr) * 0.6));
    
    // Widget must hide not destroy when closed
    g_signal_connect(G_OBJECT(_widget), 
    				 "delete-event",
    				 G_CALLBACK(gtk_widget_hide_on_delete),
    				 NULL);

	// Main dialog vbox
	GtkWidget* mainVbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(mainVbx), 
					   gtkutil::LeftAlignedLabel("<b>Objectives entities</b>"),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainVbx),
					   gtkutil::LeftAlignment(createEntitiesPanel(), 18, 1.0),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainVbx), 
					   gtkutil::LeftAlignedLabel("<b>Objectives</b>"),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainVbx),
					   gtkutil::LeftAlignment(createObjectivesPanel(), 18, 1.0),
					   TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(mainVbx), createButtons(), FALSE, FALSE, 0);
					   
	// Add vbox to dialog
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), mainVbx);
}

// Create the objects panel (for manipulating the target_addobjectives objects)
GtkWidget* ObjectivesEditor::createEntitiesPanel() {
	
	// Hbox containing the entity list and the buttons vbox
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	
	// Tree view listing the target_addobjectives entities
	GtkWidget* tv = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_objectiveEntityList));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);
	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed",
					 G_CALLBACK(_onEntitySelectionChanged), this);
	_widgets["entityList"] = tv;
	
	// Active-at-start column (checkbox)
	GtkCellRenderer* startToggle = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn* startCol = 
		gtk_tree_view_column_new_with_attributes(
			"Start", startToggle, "active", 1, NULL);
	g_signal_connect(G_OBJECT(startToggle), "toggled", 
					 G_CALLBACK(_onStartActiveCellToggled), this);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), startCol);
	
	// Name column
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("", 0));
	
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0);
					   
	// Vbox for the buttons
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 6);
	
	GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(
		G_OBJECT(addButton), "clicked", G_CALLBACK(_onAddEntity), this);
	gtk_box_pack_start(GTK_BOX(buttonBox), addButton, TRUE, TRUE, 0);
	
	GtkWidget* delButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(delButton, FALSE); // disabled at start
	g_signal_connect(
		G_OBJECT(delButton), "clicked", G_CALLBACK(_onDeleteEntity), this);
	gtk_box_pack_start(GTK_BOX(buttonBox), delButton, TRUE, TRUE, 0);
	_widgets["deleteEntity"] = delButton;
					   
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
	return hbx;
}

// Create the main objective editing widgets
GtkWidget* ObjectivesEditor::createObjectivesPanel() {
	
	// Tree view
	GtkWidget* tv = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_objectiveList));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);
	
	// Key and value text columns
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("Description", 1));
	
	return gtkutil::ScrolledFrame(tv);
}

// Create the buttons panel
GtkWidget* ObjectivesEditor::createButtons () {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(
		G_OBJECT(okButton), "clicked", G_CALLBACK(_onCancel), this);
	g_signal_connect(
		G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

// Show the dialog
void ObjectivesEditor::show() {
	gtk_widget_show_all(_widget);
	populateWidgets();
}

// Populate widgets with map data
void ObjectivesEditor::populateWidgets() {

	// Clear the selection and then the list itself
	gtk_list_store_clear(_objectiveEntityList);

	// Use an ObjectiveEntityFinder to walk the map and add any objective
	// entities to the list
	ObjectiveEntityFinder finder(_objectiveEntityList, OBJECTIVE_ENTITY_CLASS);
	GlobalSceneGraph().traverse(finder);
	
	// Set the worldspawn entity and populate the active-at-start column
	_worldSpawn = finder.getWorldSpawn();
	if (_worldSpawn != NULL)
		populateActiveAtStart();
}

// Populate the active-at-start column.
void ObjectivesEditor::populateActiveAtStart() {
	
	// Construct the list of entities targeted by the worldspawn
	TargetList targets(_worldSpawn);
	
	// Iterate through each row in the entity list. For each Entity*, get its
	// name and check if the worldspawn entity has a "target" key for this
	// entity name. This indicates that the objective entity will be active at
	// game start.
	GtkTreeIter iter;
	GtkTreeModel* model = GTK_TREE_MODEL(_objectiveEntityList);
	
	// Get the initial iter, which may return FALSE if the tree is empty
	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;
		
	// Otherwise, iterate over each row, checking for the target each time
	do {
		// Retrieve the entity pointer
		Entity* ent;
		gtk_tree_model_get(model, &iter, 2, &ent, -1);
		
		// Test if the worldspawn is targeting this entity
		if (targets.isTargeted(ent)) {
			gtk_list_store_set(_objectiveEntityList, &iter, 1, TRUE, -1);
		}
	}
	while (gtk_tree_model_iter_next(model, &iter));
}

// Static method to display dialog
void ObjectivesEditor::displayDialog() {
	
	// Static dialog instance
	static ObjectivesEditor _instance;
	
	// Show the instance
	_instance.show();
}

// Populate the objective tree
void ObjectivesEditor::populateObjectiveTree(Entity* entity) {
	
	// Clear the tree store
	gtk_list_store_clear(_objectiveList);
	
	// Use a visitor object to populate the tree store. This happens in two
	// stages, because the visitor must see all of the objective keys to build
	// a map of Objective objects, which are then inserted into the tree.
	ObjectiveKeyExtractor visitor;
	entity->forEachKeyValue(visitor);
	visitor.populateList(_objectiveList);
}

/* GTK CALLBACKS */

void ObjectivesEditor::_onCancel(GtkWidget* w, ObjectivesEditor* self) {
	gtk_widget_hide(self->_widget);
}

// Callback for "start active" cell toggle in entities list
void ObjectivesEditor::_onStartActiveCellToggled(GtkCellRendererToggle* w,
												 const gchar* path,
												 ObjectivesEditor* self)
{
	// Get the relevant row
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(
		GTK_TREE_MODEL(self->_objectiveEntityList), &iter, path);
	
	// Toggle the state of the column
	gboolean current;
	gtk_tree_model_get(GTK_TREE_MODEL(self->_objectiveEntityList), &iter, 
					   1, &current, -1);
	gtk_list_store_set(self->_objectiveEntityList, &iter, 1, !current, -1);
}

// Callback for objective entity selection changed in list box
void ObjectivesEditor::_onEntitySelectionChanged(GtkTreeSelection* sel,
												 ObjectivesEditor* self)
{
	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
	
		// Get the Entity*
		Entity* entity;
		gtk_tree_model_get(model, &iter, 2, &entity, -1);
		
		// Populate the tree
		assert(entity);
		self->populateObjectiveTree(entity);
		
		// Enable the delete button
		gtk_widget_set_sensitive(self->_widgets["deleteEntity"], TRUE); 
	}
	else {
		// No selection, disable the delete button and clear the objective
		// panel
		gtk_widget_set_sensitive(self->_widgets["deleteEntity"], FALSE);
	} 
		
}

// Add a new objectives entity button
void ObjectivesEditor::_onAddEntity(GtkWidget* w, ObjectivesEditor* self) {
	
	// Obtain the entity class object
	IEntityClassPtr eclass = 
		GlobalEntityClassManager().findOrInsert(OBJECTIVE_ENTITY_CLASS, false);
		
	// Construct a Node of this entity type
	NodeSmartReference node(GlobalEntityCreator().createEntity(eclass));
	
	// Create a random offset
	Node_getEntity(node)->setKeyValue("origin", RandomOrigin::generate(128));
	
	// Insert the node into the scene graph
	scene::Traversable* root = Node_getTraversable(GlobalSceneGraph().root());
	assert(root);
	root->insert(node);
	
	// Refresh the widgets
	self->populateWidgets();
}

// Delete entity button
void ObjectivesEditor::_onDeleteEntity(GtkWidget* w, ObjectivesEditor* self) {
	
	// Get the tree selection
	GtkTreeSelection* sel = 
		gtk_tree_view_get_selection(
			GTK_TREE_VIEW(self->_widgets["entityList"]));
			
	// Get the Node* from the tree model and remove it from the scenegraph
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		
		// Retrieve the Node*
		scene::Node* node;
		gtk_tree_model_get(model, &iter, 3, &node, -1);
		
		// Remove from scene graph
		Node_getTraversable(GlobalSceneGraph().root())->erase(*node);
		
		// Update the widgets to remove the selection from the list
		self->populateWidgets();
	}
}

}

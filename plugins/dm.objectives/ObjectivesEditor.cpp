#include "ObjectivesEditor.h"
#include "ObjectiveEntityFinder.h"
#include "RandomOrigin.h"
#include "TargetList.h"
#include "ComponentsDialog.h"
#include "MissionLogicDialog.h"
#include "util/ObjectivesException.h"

#include "iscenegraph.h"
#include "iradiant.h"
#include "iregistry.h"
#include "ieclass.h"
#include "ientity.h"

#include "scenelib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog.h"

#include <gtk/gtk.h>
#include <boost/lexical_cast.hpp>

namespace objectives
{

// CONSTANTS 
namespace {

	const char* DIALOG_TITLE = "Mission objectives"; 	
	const char* OBJECTIVE_ENTITY_CLASS = "target_tdm_addobjectives";

	const std::string RKEY_ROOT = "user/ui/objectivesEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	
	// WIDGETS ENUM 
	enum {
		WIDGET_ENTITY_LIST,
		WIDGET_LOGIC_PANEL,
		WIDGET_DELETE_ENTITY,
		WIDGET_DELETE_OBJECTIVE,
		WIDGET_EDIT_OBJECTIVE,
		WIDGET_CLEAR_OBJECTIVES,
		WIDGET_MISSION_SUCCESS_LOGIC,
		WIDGET_MISSION_FAILURE_LOGIC,
	};
}

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor() :
	gtkutil::BlockingTransientWindow(DIALOG_TITLE, GlobalRadiant().getMainWindow()),
	_objectiveEntityList(gtk_list_store_new(3, 
  										  G_TYPE_STRING, 		// display text
  										  G_TYPE_BOOLEAN,		// start active
  										  G_TYPE_STRING)),		// entity name
	_objectiveList(gtk_list_store_new(3, 
  								    G_TYPE_INT,			// obj number 
  								    G_TYPE_STRING,		// obj description
									G_TYPE_STRING))		// obj difficulty level
{
	// Window properties
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
    
    // Window size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(getWindow()));
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), 
		gint(gdk_screen_get_width(scr) * 0.5f), 
		gint(gdk_screen_get_height(scr) * 0.6)
	);
    
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
	gtk_box_pack_start(GTK_BOX(mainVbx), 
					   gtkutil::LeftAlignedLabel("<b>Success/Failure Logic</b>"),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainVbx),
					   gtkutil::LeftAlignment(createLogicPanel(), 18, 1.0),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainVbx), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(mainVbx), createButtons(), FALSE, FALSE, 0);
					   
	// Add vbox to dialog
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), mainVbx);

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
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
	_widgets[WIDGET_ENTITY_LIST] = tv;
	
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
	_widgets[WIDGET_DELETE_ENTITY] = delButton;
					   
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
	return hbx;
}

// Create the main objective editing widgets
GtkWidget* ObjectivesEditor::createObjectivesPanel() {
	
	// Tree view
	GtkWidget* tv = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_objectiveList));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);
	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed", 
					 G_CALLBACK(_onObjectiveSelectionChanged), this);
	
	// Key and value text columns
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("Description", 1, false));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("Diff.", 2, false));
	
	// Beside the list is an vbox containing add, edit, delete and clear buttons
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 6);
	
	GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD); 
	g_signal_connect(G_OBJECT(addButton), "clicked",
					 G_CALLBACK(_onAddObjective), this);

	GtkWidget* editButton = gtk_button_new_from_stock(GTK_STOCK_EDIT); 
	gtk_widget_set_sensitive(editButton, FALSE); // not enabled without selection 
	g_signal_connect(G_OBJECT(editButton), "clicked",
					 G_CALLBACK(_onEditObjective), this);
	_widgets[WIDGET_EDIT_OBJECTIVE] = editButton;
	
	GtkWidget* delButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(delButton, FALSE); // not enabled without selection 
	g_signal_connect(G_OBJECT(delButton), "clicked",
					 G_CALLBACK(_onDeleteObjective), this);
	_widgets[WIDGET_DELETE_OBJECTIVE] = delButton;
	
	GtkWidget* clearButton = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_widget_set_sensitive(clearButton, FALSE); // requires >0 objectives
	g_signal_connect(G_OBJECT(clearButton), "clicked",
					 G_CALLBACK(_onClearObjectives), this);
	_widgets[WIDGET_CLEAR_OBJECTIVES] = clearButton;
	
	gtk_box_pack_start(GTK_BOX(buttonBox), addButton, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), editButton, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), delButton, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), clearButton, FALSE, FALSE, 0);

	// Pack the list and the buttons into an hbox
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
					   
	return hbx; 
}

GtkWidget* ObjectivesEditor::createLogicPanel() {
	// Align the widgets in a table
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	gtk_widget_set_sensitive(hbox, FALSE);

	GtkWidget* editLogicButton = gtk_button_new_with_label("Edit mission success/failure logic..."); 
	gtk_button_set_image(GTK_BUTTON(editLogicButton), 
						 gtk_image_new_from_stock(GTK_STOCK_EDIT, 
						 						  GTK_ICON_SIZE_BUTTON));

	g_signal_connect(G_OBJECT(editLogicButton), "clicked", 
					 G_CALLBACK(_onEditLogic), this);

	gtk_box_pack_start(GTK_BOX(hbox), editLogicButton, TRUE, TRUE, 0);

	_widgets[WIDGET_LOGIC_PANEL] = hbox;

	return hbox;
}

// Create the buttons panel
GtkWidget* ObjectivesEditor::createButtons () {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

void ObjectivesEditor::clear() {
	// Clear internal data
	_worldSpawn = NULL;
	_entities.clear();
	_curEntity = _entities.end();

	// Clear the list boxes
	gtk_list_store_clear(_objectiveEntityList);
	gtk_list_store_clear(_objectiveList);
}

// Populate widgets with map data
void ObjectivesEditor::populateWidgets() {
	// Clear internal data first
	clear();

	// Use an ObjectiveEntityFinder to walk the map and add any objective
	// entities to the liststore and entity map
	ObjectiveEntityFinder finder(_objectiveEntityList, 
								 _entities, 
								 OBJECTIVE_ENTITY_CLASS);
	GlobalSceneGraph().traverse(finder);
	
	// Set the worldspawn entity and populate the active-at-start column
	_worldSpawn = finder.getWorldSpawn();
	if (_worldSpawn != NULL) {
		populateActiveAtStart();
	}
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
	if (!gtk_tree_model_get_iter_first(model, &iter)) {
		return;
	}
		
	// Otherwise, iterate over each row, checking for the target each time
	do {
		// Get the entity name and find it in the map
		std::string name = gtkutil::TreeModel::getString(model, &iter, 2);
		ObjectiveEntityPtr obj = _entities[name];
		
		// Test if the worldspawn is targeting this entity by passing the
		// target list to the objective entity.
		if (obj->isOnTargetList(targets)) {
			gtk_list_store_set(_objectiveEntityList, &iter, 1, TRUE, -1);
		}
	}
	while (gtk_tree_model_iter_next(model, &iter));
}

void ObjectivesEditor::_preHide() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);

	// Clear all data before hiding
	clear();
}

void ObjectivesEditor::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();
}

// Static method to display dialog
void ObjectivesEditor::displayDialog() {
	// Create a new dialog instance
	ObjectivesEditor _instance;
	
	try {
		// Show the instance
		_instance.show();
	}
	catch (ObjectivesException e) {
		gtkutil::errorDialog(
			std::string("Exception occurred: ") + e.what(), 
			GlobalRadiant().getMainWindow()
		);

		if (GTK_IS_WIDGET(_instance.getWindow())) {
			_instance.destroy();
		}
	}
}

// Refresh the objectives list from the ObjectiveEntity
void ObjectivesEditor::refreshObjectivesList() {
	
	// Clear and refresh the objective list
	gtk_list_store_clear(_objectiveList);
	_curEntity->second->populateListStore(_objectiveList);
	
	// If there is at least one objective, make the Clear button available
	if (_curEntity->second->isEmpty()) {
		gtk_widget_set_sensitive(_widgets[WIDGET_CLEAR_OBJECTIVES], FALSE);
	}
	else {
		gtk_widget_set_sensitive(_widgets[WIDGET_CLEAR_OBJECTIVES], TRUE);
	}
}

// Get the currently selected objective
Objective& ObjectivesEditor::getCurrentObjective() {
	
	// Get the objective index from the list
	int iNum;
	gtk_tree_model_get(
		GTK_TREE_MODEL(_objectiveList), &_curObjective, 0, &iNum, -1);
		
	// Pass the index to the ObjectiveEntity to get an actual Objective
	return _curEntity->second->getObjective(iNum);
}

/* GTK CALLBACKS */

void ObjectivesEditor::_onCancel(GtkWidget* w, ObjectivesEditor* self) {
	// Close the window without saving
	self->destroy();
}

// OK button
void ObjectivesEditor::_onOK(GtkWidget* w, ObjectivesEditor* self) {
	
	// Write all ObjectiveEntity data to the underlying entities
	for (ObjectiveEntityMap::iterator i = self->_entities.begin();
		 i != self->_entities.end();
		 ++i)
	{
		i->second->writeToEntity();		
	} 
	
	// Close the window
	self->destroy();
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
	// Clear the objectives list
	gtk_list_store_clear(self->_objectiveList);
	
	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
	
		// Get name of the entity and find the corresponding ObjectiveEntity in
		// the map
		std::string name = gtkutil::TreeModel::getString(model, &iter, 2);
		
		// Save the current selection and refresh the objectives list
		self->_curEntity = self->_entities.find(name);
		self->refreshObjectivesList();
		
		// Enable the delete button and objectives panel
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ENTITY], TRUE); 
		gtk_widget_set_sensitive(self->_widgets[WIDGET_LOGIC_PANEL], TRUE);
	}
	else {
		// No selection, disable the delete button and clear the objective
		// panel
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ENTITY], FALSE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_LOGIC_PANEL], FALSE);
	}
}

// Callback for current objective selection changed
void ObjectivesEditor::_onObjectiveSelectionChanged(GtkTreeSelection* sel, 
											 		ObjectivesEditor* self)
{
	// Get the selection
	if (gtk_tree_selection_get_selected(sel, NULL, &(self->_curObjective))) {
		
		// Enable the edit panel and delete button
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_OBJECTIVE], TRUE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_OBJECTIVE], TRUE);		
	}
	else {
		// Disable the edit panel and delete button
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_OBJECTIVE], FALSE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_OBJECTIVE], FALSE);
	}
}

// Add a new objectives entity button
void ObjectivesEditor::_onAddEntity(GtkWidget* w, ObjectivesEditor* self) {
	
	// Obtain the entity class object
	IEntityClassPtr eclass = 
		GlobalEntityClassManager().findClass(OBJECTIVE_ENTITY_CLASS);
		
    if (eclass) 
    {
        // Construct a Node of this entity type
        scene::INodePtr node(GlobalEntityCreator().createEntity(eclass));
        
        // Create a random offset
        Node_getEntity(node)->setKeyValue("origin", RandomOrigin::generate(128));
        
        // Insert the node into the scene graph
        assert(GlobalSceneGraph().root());
        GlobalSceneGraph().root()->addChildNode(node);
        
        // Refresh the widgets
        self->populateWidgets();
    }
    else 
    {
        // Objective entityclass was not found
        gtkutil::errorDialog(
            std::string("Unable to create Objective Entity: class '")
                + OBJECTIVE_ENTITY_CLASS + "' not found.",
            GlobalRadiant().getMainWindow()
        );
    }
}

// Delete entity button
void ObjectivesEditor::_onDeleteEntity(GtkWidget* w, ObjectivesEditor* self) {
	
	// Get the tree selection
	GtkTreeSelection* sel = 
		gtk_tree_view_get_selection(
			GTK_TREE_VIEW(self->_widgets[WIDGET_ENTITY_LIST]));
			
	// Get the Node* from the tree model and remove it from the scenegraph
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		
		// Get the name of the selected entity
		std::string name = gtkutil::TreeModel::getString(model, &iter, 2);
		
		// Instruct the ObjectiveEntity to delete its world node, and then
		// remove it from the map
		self->_entities[name]->deleteWorldNode();
		self->_entities.erase(name);

		// Update the widgets to remove the selection from the list
		self->populateWidgets();
	}
}

// Add a new objective
void ObjectivesEditor::_onAddObjective(GtkWidget* w, ObjectivesEditor* self) {
	// Add a new objective to the ObjectiveEntity and refresh the list store
	self->_curEntity->second->addObjective();
	self->refreshObjectivesList();	
}

// Edit an existing objective
void ObjectivesEditor::_onEditObjective(GtkWidget* w, ObjectivesEditor* self) {
	// Display the ComponentsDialog
	ComponentsDialog compDialog(GTK_WINDOW(self->getWindow()),
								self->getCurrentObjective());
	compDialog.show(); // show and block

	// Repopulate the objective list
	self->refreshObjectivesList();
}

// Delete an objective
void ObjectivesEditor::_onDeleteObjective(GtkWidget* w, 
										  ObjectivesEditor* self)
{
	// Get the index of the current objective
	int index;
	gtk_tree_model_get(GTK_TREE_MODEL(self->_objectiveList), 
					   &(self->_curObjective),
					   0, &index, -1);
					   
	// Tell the ObjectiveEntity to delete this objective
	self->_curEntity->second->deleteObjective(index);
	
	// Repopulate the objective list
	self->refreshObjectivesList();
}

// Clear the objectives
void ObjectivesEditor::_onClearObjectives(GtkWidget* w, 
										  ObjectivesEditor* self)
{
	// Clear the entity and refresh the list
	self->_curEntity->second->clearObjectives();
	self->refreshObjectivesList();
}

void ObjectivesEditor::_onEditLogic(GtkWidget* w, ObjectivesEditor* self) {
	MissionLogicDialog _dialog(GTK_WINDOW(self->getWindow()), *self->_curEntity->second);
	_dialog.show();

	self->refreshObjectivesList();
}

} // namespace objectives

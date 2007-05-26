#include "ObjectivesEditor.h"
#include "ObjectiveEntityFinder.h"
#include "RandomOrigin.h"
#include "TargetList.h"
#include "ComponentsDialog.h"

#include "iscenegraph.h"
#include "iradiant.h"
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

#include <gtk/gtk.h>
#include <boost/lexical_cast.hpp>

namespace objectives
{

/* CONSTANTS */
namespace {

	const char* DIALOG_TITLE = "Mission objectives"; 	
	const char* OBJECTIVE_ENTITY_CLASS = "target_tdm_addobjectives";
	
	/* WIDGETS ENUM */
	enum {
		WIDGET_OBJECTIVES_PANEL,
		WIDGET_ENTITY_LIST,
		WIDGET_EDIT_PANEL,
		WIDGET_DELETE_ENTITY,
		WIDGET_DELETE_OBJECTIVE,
		WIDGET_CLEAR_OBJECTIVES,
		WIDGET_DESCRIPTION_ENTRY,
		WIDGET_STATE_COMBO,
		WIDGET_MANDATORY_FLAG,
		WIDGET_IRREVERSIBLE_FLAG,
		WIDGET_ONGOING_FLAG,
		WIDGET_VISIBLE_FLAG,
		WIDGET_COMPONENTS_COUNT,
		WIDGET_COMPONENTS_BUTTON
	};
	
}

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _objectiveEntityList(gtk_list_store_new(3, 
  										  G_TYPE_STRING, 		// display text
  										  G_TYPE_BOOLEAN,		// start active
  										  G_TYPE_STRING)),		// entity name
  _objectiveList(gtk_list_store_new(2, 
  								    G_TYPE_INT,			// obj number 
  								    G_TYPE_STRING)),	// obj description
  _objectiveListLocked(false)
{
	// Window properties
	gtk_window_set_transient_for(
		GTK_WINDOW(_widget), GlobalRadiant().getMainWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), DIALOG_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(_widget), GDK_WINDOW_TYPE_HINT_DIALOG);
    
    // Window size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gtk_window_set_default_size(GTK_WINDOW(_widget), -1, 
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
	gtk_box_pack_start(GTK_BOX(mainVbx), gtk_hseparator_new(), FALSE, FALSE, 0);
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
	
	// Beside the list is an vbox containing add, delete and clear buttons
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 6);
	
	GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD); 
	g_signal_connect(G_OBJECT(addButton), "clicked",
					 G_CALLBACK(_onAddObjective), this);
	
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
	gtk_box_pack_start(GTK_BOX(buttonBox), delButton, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), clearButton, FALSE, FALSE, 0);

	// Pack the list and the buttons into an hbox
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
					   
	// Finally, pack the hbox into a vbox, with the current objective edit panel
	// underneath
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createObjectiveEditPanel(), 
					   FALSE, FALSE, 0);
	gtk_widget_set_sensitive(vbx, FALSE);
	_widgets[WIDGET_OBJECTIVES_PANEL] = vbx;
					   
	return vbx; 
}

// Create the panel for editing the currently-selected objective
GtkWidget* ObjectivesEditor::createObjectiveEditPanel() {

	// Table for entry boxes
	GtkWidget* table = gtk_table_new(4, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 12);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	
	// Objective description
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel("<b>Description</b>"),
					 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	_widgets[WIDGET_DESCRIPTION_ENTRY] = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_DESCRIPTION_ENTRY], 
							  1, 2, 0, 1);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DESCRIPTION_ENTRY]), "changed",
					 G_CALLBACK(_onDescriptionEdited), this);
	
	// State selection
	gtk_table_attach(GTK_TABLE(table),
					 gtkutil::LeftAlignedLabel("<b>Initial state</b>"),
					 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	_widgets[WIDGET_STATE_COMBO] = gtk_combo_box_new_text();
	gtk_table_attach_defaults(GTK_TABLE(table),
							  _widgets[WIDGET_STATE_COMBO],
							  1, 2, 1, 2);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_STATE_COMBO]), "changed",
					 G_CALLBACK(_onInitialStateChanged), this);

	// Populate the list of states. This must be done in order to match the
	// values in the enum, since the index will be used when writing to entity
	GtkComboBox* combo = GTK_COMBO_BOX(_widgets[WIDGET_STATE_COMBO]);
	gtk_combo_box_append_text(combo, "COMPLETE");
	gtk_combo_box_append_text(combo, "INCOMPLETE");
	gtk_combo_box_append_text(combo, "FAILED");
	gtk_combo_box_append_text(combo, "INVALID");
	
	// Options checkboxes.
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel("<b>Flags</b>"),
					 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), createFlagsTable(), 1, 2, 2, 3);
	
	// Components text and button
	GtkWidget* compButton = gtk_button_new_with_label("Edit conditions..."); 
	gtk_button_set_image(GTK_BUTTON(compButton), 
						 gtk_image_new_from_stock(GTK_STOCK_EDIT, 
						 						  GTK_ICON_SIZE_BUTTON));
	_widgets[WIDGET_COMPONENTS_BUTTON] = compButton;
	g_signal_connect(G_OBJECT(compButton), "clicked", 
					 G_CALLBACK(_onEditComponents), this);
		
	_widgets[WIDGET_COMPONENTS_COUNT] = gtk_label_new("0 condition(s)");
	
	GtkWidget* compBox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(compBox), _widgets[WIDGET_COMPONENTS_COUNT],
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(compBox), compButton, TRUE, TRUE, 0);

	gtk_table_attach(GTK_TABLE(table),
					 gtkutil::LeftAlignedLabel("<b>Conditions</b>"),
					 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), compBox, 1, 2, 3, 4);
	
	// Pack items into a vbox and return
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), table, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(vbx, FALSE);
	_widgets[WIDGET_EDIT_PANEL] = vbx;

	return vbx;
}

// Create table of flag checkboxes
GtkWidget* ObjectivesEditor::createFlagsTable() {
	
	GtkWidget* hbx = gtk_hbox_new(FALSE, 12);

	_widgets[WIDGET_MANDATORY_FLAG] =
		gtk_check_button_new_with_label("Mandatory"); 
	_widgets[WIDGET_IRREVERSIBLE_FLAG] =
		gtk_check_button_new_with_label("Irreversible"); 
	_widgets[WIDGET_ONGOING_FLAG] =
		gtk_check_button_new_with_label("Ongoing"); 
	_widgets[WIDGET_VISIBLE_FLAG] =
		gtk_check_button_new_with_label("Visible"); 

	g_signal_connect(G_OBJECT(_widgets[WIDGET_MANDATORY_FLAG]), "toggled",
					 G_CALLBACK(_onFlagToggle), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_IRREVERSIBLE_FLAG]), "toggled",
					 G_CALLBACK(_onFlagToggle), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_ONGOING_FLAG]), "toggled",
					 G_CALLBACK(_onFlagToggle), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_VISIBLE_FLAG]), "toggled",
					 G_CALLBACK(_onFlagToggle), this);

	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_MANDATORY_FLAG], 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_ONGOING_FLAG], 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_IRREVERSIBLE_FLAG], 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _widgets[WIDGET_VISIBLE_FLAG], 
					   FALSE, FALSE, 0);

	return hbx;

}

// Create the buttons panel
GtkWidget* ObjectivesEditor::createButtons () {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(
		G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
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

	// Clear internal data
	_worldSpawn = NULL;
	_entities.clear();
	_curEntity = _entities.end();

	// Clear the list boxes
	gtk_list_store_clear(_objectiveEntityList);
	gtk_list_store_clear(_objectiveList);

	// Use an ObjectiveEntityFinder to walk the map and add any objective
	// entities to the liststore and entity map
	ObjectiveEntityFinder finder(_objectiveEntityList, 
								 _entities, 
								 OBJECTIVE_ENTITY_CLASS);
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

// Static method to display dialog
void ObjectivesEditor::displayDialog() {
	
	// Static dialog instance
	static ObjectivesEditor _instance;
	
	// Show the instance
	_instance.show();
}

// Populate the edit panel widgets using the given objective number
void ObjectivesEditor::populateEditPanel() {

	// Disable updates to the list store, so that setting widget values doesn't
	// change the list data while we are in the middle of reading it.
	_objectiveListLocked = true;

	// Get the objective
	const Objective& obj = getCurrentObjective();
	
	// Set description text
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_DESCRIPTION_ENTRY]),
					   obj.description.c_str());
				
	// Set initial state enum
	gtk_combo_box_set_active(GTK_COMBO_BOX(_widgets[WIDGET_STATE_COMBO]),
							 obj.state);
					   
	// Set flags
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_IRREVERSIBLE_FLAG]),
		obj.irreversible ? TRUE : FALSE);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_ONGOING_FLAG]),
		obj.ongoing ? TRUE : FALSE);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_MANDATORY_FLAG]),
		obj.mandatory ? TRUE : FALSE);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_VISIBLE_FLAG]),
		obj.visible ? TRUE : FALSE);
		
	// Set component count
	std::string sCount = boost::lexical_cast<std::string>(obj.components.size()) 
						 + " condition(s)"; 
	gtk_label_set_text(GTK_LABEL(_widgets[WIDGET_COMPONENTS_COUNT]),
					   sCount.c_str());
		
	// Re-enable updates
	_objectiveListLocked = false;
}

// Refresh the objectives list from the ObjectiveEntity
void ObjectivesEditor::refreshObjectivesList() {
	
	// Clear and refresh the objective list
	gtk_list_store_clear(_objectiveList);
	_curEntity->second->populateListStore(_objectiveList);
	
	// If there is at least one objective, make the Clear button available
	if (_curEntity->second->isEmpty())
		gtk_widget_set_sensitive(_widgets[WIDGET_CLEAR_OBJECTIVES], FALSE);
	else
		gtk_widget_set_sensitive(_widgets[WIDGET_CLEAR_OBJECTIVES], TRUE);
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
	gtk_widget_hide(self->_widget);
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
	
	// Hide the dialog
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
		gtk_widget_set_sensitive(
			self->_widgets[WIDGET_OBJECTIVES_PANEL], TRUE);
	}
	else {
		// No selection, disable the delete button and clear the objective
		// panel
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ENTITY], FALSE);
		gtk_widget_set_sensitive(
			self->_widgets[WIDGET_OBJECTIVES_PANEL], FALSE);
	} 
		
}

// Callback for current objective selection changed
void ObjectivesEditor::_onObjectiveSelectionChanged(GtkTreeSelection* sel, 
											 		ObjectivesEditor* self)
{
	// Get the selection
	if (gtk_tree_selection_get_selected(sel, NULL, &(self->_curObjective))) {
		
		// Enable the edit panel and delete button
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_PANEL], TRUE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_OBJECTIVE], TRUE);		
		
		// Populate the edit panel
		self->populateEditPanel();				
	}
	else {
		// Disable the edit panel and delete button
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_PANEL], FALSE);		
		gtk_widget_set_sensitive(
			self->_widgets[WIDGET_DELETE_OBJECTIVE], FALSE);		
	}
}

// Add a new objectives entity button
void ObjectivesEditor::_onAddEntity(GtkWidget* w, ObjectivesEditor* self) {
	
	// Obtain the entity class object
	IEntityClassPtr eclass = 
		GlobalEntityClassManager().findOrInsert(OBJECTIVE_ENTITY_CLASS, false);
		
	// Construct a Node of this entity type
	scene::INodePtr node(GlobalEntityCreator().createEntity(eclass));
	
	// Create a random offset
	Node_getEntity(node)->setKeyValue("origin", RandomOrigin::generate(128));
	
	// Insert the node into the scene graph
	scene::TraversablePtr root = Node_getTraversable(GlobalSceneGraph().root());
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

// Callback for flag checkbox toggle
void ObjectivesEditor::_onFlagToggle(GtkWidget* flag, ObjectivesEditor* self) {
	
	// Get the objective and the new status
	Objective& o = self->getCurrentObjective();
	bool status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(flag));
	
	// Determine which checkbox is toggled, then update the appropriate flag
	// accordingly
	if (flag == self->_widgets[WIDGET_MANDATORY_FLAG])
		o.mandatory = status;
	else if (flag == self->_widgets[WIDGET_VISIBLE_FLAG])
		o.visible = status;
	else if (flag == self->_widgets[WIDGET_ONGOING_FLAG])
		o.ongoing = status;
	else if (flag == self->_widgets[WIDGET_IRREVERSIBLE_FLAG])
		o.irreversible = status;
}

// Initial state combo changed
void ObjectivesEditor::_onInitialStateChanged(GtkWidget* w, 
											  ObjectivesEditor* self)
{
	// Set the state enum value from the combo box index
	self->getCurrentObjective().state = 
		static_cast<Objective::State>(
			gtk_combo_box_get_active(GTK_COMBO_BOX(w)));
}

// Callback when description is edited
void ObjectivesEditor::_onDescriptionEdited(GtkEditable* e, 
											ObjectivesEditor* self)
{
	// Abort if the objective liststore is locked
	if (self->_objectiveListLocked)
		return;
	
	// Get the string
	std::string desc = gtk_entry_get_text(GTK_ENTRY(e));

	// Set the string on the Objective 
	Objective& o = self->getCurrentObjective();
	o.description = desc;
	
	// Update the list store
	gtk_list_store_set(self->_objectiveList, &(self->_curObjective),
					   1, desc.c_str(), -1);
}

// Callback for Edit Components button
void ObjectivesEditor::_onEditComponents(GtkWidget* w, ObjectivesEditor* self) {
	
	// Display the ComponentsDialog
	ComponentsDialog compDialog(GTK_WINDOW(self->_widget),
								self->getCurrentObjective());
	compDialog.showAndBlock();
}

}

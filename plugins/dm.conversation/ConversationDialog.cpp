#include "ConversationDialog.h"

#include "iregistry.h"
#include "iundo.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "string/string.h"

#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog.h"

#include "RandomOrigin.h"
#include "ConversationEntityFinder.h"
#include "ConversationEditor.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <iostream>

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Conversation Editor";
	
	const std::string RKEY_ROOT = "user/ui/conversationDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

	const std::string CONVERSATION_ENTITY_CLASS = "atdm:conversation_info";

	// WIDGETS ENUM 
	enum {
		WIDGET_ENTITY_LIST,
		WIDGET_DELETE_ENTITY,
		WIDGET_CONV_BUTTONS_PANEL,
		WIDGET_CLEAR_CONVERSATIONS,
		WIDGET_EDIT_CONVERSATION,
		WIDGET_DELETE_CONVERSATION,
	};
}

ConversationDialog::ConversationDialog() :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow()),
	_convEntityList(gtk_list_store_new(2, 
  									  G_TYPE_STRING, 		// display text
  									  G_TYPE_STRING)),		// entity name
	_convList(gtk_list_store_new(2, 
  								G_TYPE_INT, 		// index
  								G_TYPE_STRING))		// name
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal(GTK_WINDOW(getWindow()), TRUE);
	
	g_signal_connect(G_OBJECT(getWindow()), "key-press-event", 
					 G_CALLBACK(onWindowKeyPress), this);
	
	// Create the widgets
	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();

	// Show the dialog, this enters the gtk main loop
	show();
}

void ConversationDialog::_preHide() {
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

void ConversationDialog::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();
}

void ConversationDialog::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), _dialogVBox);
	
	gtk_box_pack_start(GTK_BOX(_dialogVBox), 
					   gtkutil::LeftAlignedLabel("<b>Conversation entities</b>"),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_dialogVBox),
					   gtkutil::LeftAlignment(createEntitiesPanel(), 18, 1.0),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_dialogVBox), 
					   gtkutil::LeftAlignedLabel("<b>Conversations</b>"),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_dialogVBox),
					   gtkutil::LeftAlignment(createConversationsPanel(), 18, 1.0),
					   TRUE, TRUE, 0);
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
}

// Create the conversation entity panel
GtkWidget* ConversationDialog::createEntitiesPanel() {
	
	// Hbox containing the entity list and the buttons vbox
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	
	// Tree view listing the conversation_info entities
	GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_convEntityList));

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);
	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed",
					 G_CALLBACK(onEntitySelectionChanged), this);
	_widgets[WIDGET_ENTITY_LIST] = tv;
	
	// Entity Name column
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("", 0));
	
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0);
					   
	// Vbox for the buttons
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 6);
	
	GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(
		G_OBJECT(addButton), "clicked", G_CALLBACK(onAddEntity), this);
	gtk_box_pack_start(GTK_BOX(buttonBox), addButton, TRUE, TRUE, 0);
	
	GtkWidget* delButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(delButton, FALSE); // disabled at start
	g_signal_connect(
		G_OBJECT(delButton), "clicked", G_CALLBACK(onDeleteEntity), this);
	gtk_box_pack_start(GTK_BOX(buttonBox), delButton, TRUE, TRUE, 0);
	_widgets[WIDGET_DELETE_ENTITY] = delButton;
					   
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
	return hbx;
}

// Create the main conversation editing widgets
GtkWidget* ConversationDialog::createConversationsPanel() {
	// Tree view
	GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_convList));

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onConversationSelectionChanged), this);
	
	// Key and value text columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("Name", 1, false));
	
	// Beside the list is an vbox containing add, edit, delete and clear buttons
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 6);
    
    // Buttons panel box is disabled by default, enabled once an Entity is selected.
    gtk_widget_set_sensitive(buttonBox, FALSE);
    _widgets[WIDGET_CONV_BUTTONS_PANEL] = buttonBox;

	GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD); 
	g_signal_connect(G_OBJECT(addButton), "clicked", G_CALLBACK(onAddConversation), this);

	GtkWidget* editButton = gtk_button_new_from_stock(GTK_STOCK_EDIT); 
	gtk_widget_set_sensitive(editButton, FALSE); // not enabled without selection 
	g_signal_connect(G_OBJECT(editButton), "clicked", G_CALLBACK(onEditConversation), this);
	_widgets[WIDGET_EDIT_CONVERSATION] = editButton;
	
	GtkWidget* delButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(delButton, FALSE); // not enabled without selection 
	g_signal_connect(G_OBJECT(delButton), "clicked", G_CALLBACK(onDeleteConversation), this);
	_widgets[WIDGET_DELETE_CONVERSATION] = delButton;
	
	GtkWidget* clearButton = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_widget_set_sensitive(clearButton, FALSE); // requires >0 conversations
	g_signal_connect(G_OBJECT(clearButton), "clicked", G_CALLBACK(onClearConversations), this);
	_widgets[WIDGET_CLEAR_CONVERSATIONS] = clearButton;
	
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

// Lower dialog buttons
GtkWidget* ConversationDialog::createButtons() {

	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Save button
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onSave), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
	
	// Close Button
	_closeButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(
		G_OBJECT(_closeButton), "clicked", G_CALLBACK(onClose), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), _closeButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

void ConversationDialog::save() {
	// Consistency check can go here
	
	// Scoped undo object
	UndoableCommand command("editConversations");
	
	// Save the working set to the entity
	for (conversation::ConversationEntityMap::iterator i = _entities.begin();
		 i != _entities.end();
		 ++i)
	{
		i->second->writeToEntity();		
	}
}

void ConversationDialog::clear() {
	// Clear internal data
	_entities.clear();
	_curEntity = _entities.end();

	// Clear the list boxes
	gtk_list_store_clear(_convEntityList);
	gtk_list_store_clear(_convList);
}

void ConversationDialog::refreshConversationList() {
	// Clear and refresh the conversation list
	gtk_list_store_clear(_convList);
	_curEntity->second->populateListStore(_convList);
	
	// If there is at least one conversation, make the Clear button available
	gtk_widget_set_sensitive(
		_widgets[WIDGET_CLEAR_CONVERSATIONS],
		_curEntity->second->isEmpty() ? FALSE : TRUE
	);
}

void ConversationDialog::populateWidgets() {
	// First clear the data
	clear();

	// Use an ConversationEntityFinder to walk the map and add any conversation
	// entities to the liststore and entity map
	conversation::ConversationEntityFinder finder(
		_convEntityList, 
		_entities, 
		CONVERSATION_ENTITY_CLASS
	);

	GlobalSceneGraph().root()->traverse(finder);
}

void ConversationDialog::onSave(GtkWidget* button, ConversationDialog* self) {
	self->save();
	self->destroy();
}

void ConversationDialog::onClose(GtkWidget* button, ConversationDialog* self) {
	self->destroy();
}

gboolean ConversationDialog::onWindowKeyPress(
	GtkWidget* dialog, GdkEventKey* event, ConversationDialog* self)
{
	if (event->keyval == GDK_Escape) {
		self->destroy();
		// Catch this keyevent, don't propagate
		return TRUE;
	}
	
	// Propagate further
	return FALSE;
}

// Static command target
void ConversationDialog::showDialog(const cmd::ArgumentList& args) {
	// Construct a new instance, this enters the main loop
	ConversationDialog _editor;
}

// Callback for conversation entity selection changed in list box
void ConversationDialog::onEntitySelectionChanged(GtkTreeSelection* sel,
												 ConversationDialog* self)
{
	// Clear the conversations list
	gtk_list_store_clear(self->_convList);
	
	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) 
    {
		// Get name of the entity and find the corresponding ConversationEntity in the map
		std::string name = gtkutil::TreeModel::getString(model, &iter, 1);
		
		// Save the current selection and refresh the conversation list
		self->_curEntity = self->_entities.find(name);
		self->refreshConversationList();
		
		// Enable the delete button and conversation panel
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ENTITY], TRUE); 
		gtk_widget_set_sensitive(self->_widgets[WIDGET_CONV_BUTTONS_PANEL], TRUE);
	}
	else 
    {
		// No selection, disable the delete button and clear the conversation panel
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ENTITY], FALSE);
		
        // Disable all the Conversation edit buttons
        gtk_widget_set_sensitive(self->_widgets[WIDGET_CONV_BUTTONS_PANEL], FALSE);
	}
}

// Add a new conversations entity button
void ConversationDialog::onAddEntity(GtkWidget* w, ConversationDialog* self) {
	
	// Obtain the entity class object
	IEntityClassPtr eclass = 
		GlobalEntityClassManager().findClass(CONVERSATION_ENTITY_CLASS);
		
    if (eclass != NULL) 
    {
        // Construct a Node of this entity type
        scene::INodePtr node(GlobalEntityCreator().createEntity(eclass));
        
        // Create a random offset
		Node_getEntity(node)->setKeyValue("origin", conversation::RandomOrigin::generate(128));
        
        // Insert the node into the scene graph
        assert(GlobalSceneGraph().root());
        GlobalSceneGraph().root()->addChildNode(node);
        
        // Refresh the widgets
        self->populateWidgets();
    }
    else 
    {
        // conversation entityclass was not found
        gtkutil::errorDialog(
            std::string("Unable to create conversation Entity: class '")
                + CONVERSATION_ENTITY_CLASS + "' not found.",
            GlobalRadiant().getMainWindow()
        );
    }
}

// Delete entity button
void ConversationDialog::onDeleteEntity(GtkWidget* w, ConversationDialog* self) {
	// Get the tree selection
	GtkTreeSelection* sel = 
		gtk_tree_view_get_selection(
			GTK_TREE_VIEW(self->_widgets[WIDGET_ENTITY_LIST]));
			
	// Get the Node* from the tree model and remove it from the scenegraph
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		
		// Get the name of the selected entity
		std::string name = gtkutil::TreeModel::getString(model, &iter, 1);
		
		// Instruct the ConversationEntity to delete its world node, and then
		// remove it from the map
		self->_entities[name]->deleteWorldNode();
		self->_entities.erase(name);

		// Update the widgets to remove the selection from the list
		self->populateWidgets();
	}
}

// Callback for current conversation selection changed
void ConversationDialog::onConversationSelectionChanged(GtkTreeSelection* sel, 
											 		ConversationDialog* self)
{
	// Get the selection
	if (gtk_tree_selection_get_selected(sel, NULL, &(self->_currentConversation))) {
		// Enable the edit and delete buttons
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_CONVERSATION], TRUE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_CONVERSATION], TRUE);		
	}
	else {
		// Disable the edit and delete buttons
		gtk_widget_set_sensitive(self->_widgets[WIDGET_EDIT_CONVERSATION], FALSE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_CONVERSATION], FALSE);
	}
}

void ConversationDialog::onAddConversation(GtkWidget*, ConversationDialog* self) {
	// Add a new conversation to the ConversationEntity and refresh the list store
	self->_curEntity->second->addConversation();
	self->refreshConversationList();
}

void ConversationDialog::onEditConversation(GtkWidget*, ConversationDialog* self) {
	// Retrieve the index of the current conversation
	int index = gtkutil::TreeModel::getInt(
		GTK_TREE_MODEL(self->_convList), 
		&(self->_currentConversation), 
		0 // column number
	);

	conversation::Conversation& conv = self->_curEntity->second->getConversation(index);

	// Display the edit dialog, blocks on construction
	ConversationEditor editor(GTK_WINDOW(self->getWindow()), conv);

	// Repopulate the conversation list
	self->refreshConversationList();
}

void ConversationDialog::onDeleteConversation(GtkWidget*, ConversationDialog* self) {
	// Get the index of the current conversation
	int index;
	gtk_tree_model_get(GTK_TREE_MODEL(self->_convList), 
					   &(self->_currentConversation),
					   0, &index, -1);

	// Tell the ConversationEntity to delete this conversation
	self->_curEntity->second->deleteConversation(index);
	
	// Repopulate the conversation list
	self->refreshConversationList();
}

void ConversationDialog::onClearConversations(GtkWidget*, ConversationDialog* self) {
	// Clear the entity and refresh the list
	self->_curEntity->second->clearConversations();
	self->refreshConversationList();
}

} // namespace ui

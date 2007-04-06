#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "selectionlib.h"
#include "gtkutil/image.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/dialog.h"
#include "string/string.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "EffectEditor.h"

#include <iostream>

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Stim/Response Editor";
	
	const std::string RKEY_ROOT = "user/ui/stimResponseEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	
	const std::string LABEL_ADD_STIMRESPONSE = "<b>Add Stim/Response</b>";
	const std::string LABEL_RESPONSE_EFFECTS = "<b>Response Effects</b>";
	
	const char* LABEL_SAVE = "Save to Entity";
	const char* LABEL_REVERT = "Reload from Entity";
	
	const char* NO_ENTITY_ERROR = "A single entity must be selected to edit "
								  "Stim/Response properties.";
	
	const unsigned int TREE_VIEW_WIDTH = 280;
	const unsigned int TREE_VIEW_HEIGHT = 240;
	const unsigned int OPTIONS_LABEL_WIDTH = 140; 
}

StimResponseEditor::StimResponseEditor() :
	_entity(NULL),
	_updatesDisabled(false),
	_stimEditor(_stimTypes),
	_responseEditor(_stimTypes)
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_modal(GTK_WINDOW(_dialog), TRUE);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", 
					 G_CALLBACK(onDelete), this);
	g_signal_connect(G_OBJECT(_dialog), "key-press-event", 
					 G_CALLBACK(onWindowKeyPress), this);
	
	// Create the widgets
	populateWindow();
	createContextMenus();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

void StimResponseEditor::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	gtk_widget_hide(_dialog);
}

void StimResponseEditor::toggleWindow() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		// Save the window position, to make sure
		_windowPosition.readPosition();
		gtk_widget_hide_all(_dialog);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Scan the selection for entities
		rescanSelection();
		
		// Has the rescan found an entity (the pointer is non-NULL then)
		if (_entity != NULL) {
			// Now show the dialog window again
			gtk_widget_show_all(_dialog);
		}
		else {
			gtkutil::errorDialog(NO_ENTITY_ERROR, 
								 GlobalRadiant().getMainWindow());
			gtk_widget_hide_all(_dialog);
		}
	}
}

void StimResponseEditor::populateWindow() {
	
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(_dialog), _dialogVBox);
	
	// Create the notebook and add it to the vbox
	_notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(_notebook), TRUE, TRUE, 0);
	
	// The tab label items (icon + label)
	GtkWidget* stimLabelHBox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(
    	GTK_BOX(stimLabelHBox), 
    	gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_STIM)), 
    	FALSE, FALSE, 3
    );
	gtk_box_pack_start(GTK_BOX(stimLabelHBox), gtk_label_new("Stims"), FALSE, FALSE, 3);
	
	GtkWidget* responseLabelHBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(
    	GTK_BOX(responseLabelHBox), 
    	gtk_image_new_from_pixbuf(gtkutil::getLocalPixbufWithMask(ICON_RESPONSE)), 
    	FALSE, FALSE, 0
    );
	gtk_box_pack_start(GTK_BOX(responseLabelHBox), gtk_label_new("Responses"), FALSE, FALSE, 3);
	
	// Show the widgets before using them as label, they won't appear otherwise
	gtk_widget_show_all(stimLabelHBox);
	gtk_widget_show_all(responseLabelHBox);
	
	// Cast the helper class to a widget and add it to the notebook page
	_stimPageNum = gtk_notebook_append_page(_notebook, _stimEditor, stimLabelHBox);
	_responsePageNum = gtk_notebook_append_page(_notebook, _responseEditor, responseLabelHBox);
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
}

// Lower dialog buttons
GtkWidget* StimResponseEditor::createButtons() {

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

// Create the response script list widgets
GtkWidget* StimResponseEditor::createEffectWidgets() {

	_effectWidgets.view = gtk_tree_view_new();
	_effectWidgets.selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_effectWidgets.view));
	gtk_widget_set_size_request(_effectWidgets.view, -1, 200);
	
	// Connect the signals
	g_signal_connect(G_OBJECT(_effectWidgets.selection), "changed",
					 G_CALLBACK(onEffectSelectionChange), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "button-press-event",
					 G_CALLBACK(onTreeViewButtonPress), this);
	g_signal_connect(G_OBJECT(_effectWidgets.view), "button-release-event",
					 G_CALLBACK(onTreeViewButtonRelease), this);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("#", EFFECT_INDEX_COL)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("Effect", EFFECT_CAPTION_COL)
	);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_effectWidgets.view), 
		gtkutil::TextColumn("Description", EFFECT_ARGS_COL)
	);
	
	// Return the tree view in a frame
	return gtkutil::ScrolledFrame(_effectWidgets.view);
}

// Create the context menus
void StimResponseEditor::createContextMenus() {
	
	// Menu widgets
	_stimListContextMenu = gtk_menu_new();
	_effectListContextMenu = gtk_menu_new();
	
	// Each menu gets a delete item
	_srWidgets.deleteMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
														   "Delete");
	_srWidgets.addMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
														   "Add new S/R");
	gtk_menu_shell_append(GTK_MENU_SHELL(_stimListContextMenu), 
						  _srWidgets.addMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_stimListContextMenu), 
						  _srWidgets.deleteMenuItem);

	_effectWidgets.addMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_ADD,
															   "Add new Effect");
	_effectWidgets.deleteMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_DELETE,
															   "Delete");
	_effectWidgets.upMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_GO_UP,
															   "Move Up");
	_effectWidgets.downMenuItem = gtkutil::StockIconMenuItem(GTK_STOCK_GO_DOWN,
															   "Move Down");
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.addMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.upMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.downMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(_effectListContextMenu), 
						  _effectWidgets.deleteMenuItem);

	// Connect up the signals
	g_signal_connect(G_OBJECT(_srWidgets.deleteMenuItem), "activate",
					 G_CALLBACK(_onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_srWidgets.addMenuItem), "activate",
					 G_CALLBACK(_onContextMenuAdd), this);
					 
	g_signal_connect(G_OBJECT(_effectWidgets.deleteMenuItem), "activate",
					 G_CALLBACK(_onContextMenuDelete), this);
	g_signal_connect(G_OBJECT(_effectWidgets.addMenuItem), "activate",
					 G_CALLBACK(_onContextMenuAdd), this);
	g_signal_connect(G_OBJECT(_effectWidgets.upMenuItem), "activate",
					 G_CALLBACK(_onContextMenuEffectUp), this);
	g_signal_connect(G_OBJECT(_effectWidgets.downMenuItem), "activate",
					 G_CALLBACK(_onContextMenuEffectDown), this);

	// Show menus (not actually visible until popped up)
	gtk_widget_show_all(_stimListContextMenu);
	gtk_widget_show_all(_effectListContextMenu);
}

void StimResponseEditor::update() {
	gtk_widget_set_sensitive(_dialogVBox, _entity != NULL);
	
	updateSRWidgets();
	gtk_widget_set_sensitive(_closeButton, TRUE);
}

void StimResponseEditor::rescanSelection() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = NULL;
	_srEntity = SREntityPtr();
	_stimEditor.setEntity(_srEntity);
	_responseEditor.setEntity(_srEntity);
	
	if (info.entityCount == 1 && info.totalCount == 1) {
		// Get the entity instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& node = instance.path().top();
		
		_entity = Node_getEntity(node);
		
		_srEntity = SREntityPtr(new SREntity(_entity));
		_stimEditor.setEntity(_srEntity);
		_responseEditor.setEntity(_srEntity);
		
		// Clear the treeview (unset the model)
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), NULL);
	}
	
	if (_entity != NULL) {
		std::string title = WINDOW_TITLE + " (" + _entity->getKeyValue("name") + ")";
		gtk_window_set_title(GTK_WINDOW(_dialog), title.c_str()); 
	}
	else {
		gtk_window_set_title(GTK_WINDOW(_dialog), WINDOW_TITLE.c_str());
	}
	
	// Update the widgets
	update();
}

void StimResponseEditor::updateSRWidgets() {
	_updatesDisabled = true;
	
	int id = getIdFromSelection();
	
	if (id > 0 && _srEntity != NULL) {
		// Update all the widgets
		gtk_widget_set_sensitive(_srWidgets.vbox, TRUE);
		
		StimResponse& sr = _srEntity->get(id);
		
		GtkListStore* effectStore = sr.getEffectStore();
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), GTK_TREE_MODEL(effectStore));
		g_object_unref(effectStore);
		
		// Allow effect editing for non-inherited responses only
		gtk_widget_set_sensitive(
			_effectWidgets.view, 
			(sr.get("class") == "R" && !sr.inherited())
		);
		
		// The response effect list may be empty, so force an update of the
		// context menu sensitivity, in the case the "selection changed" 
		// signal doesn't get called
		updateEffectContextMenu();
	}
	else {
		gtk_widget_set_sensitive(_srWidgets.vbox, FALSE);
		
		// Clear the effect tree view
		gtk_tree_view_set_model(GTK_TREE_VIEW(_effectWidgets.view), NULL);
	}
	
	_updatesDisabled = false;
}

void StimResponseEditor::addEffect() {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response
		if (sr.get("class") == "R") {
			// Add a new effect and update all the widgets
			sr.addEffect(effectIndex);
			update();
		}
	}
}

void StimResponseEditor::removeEffect() {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Remove the effect and update all the widgets
			sr.deleteEffect(effectIndex);
			update();
		}
	}
}

void StimResponseEditor::selectEffectIndex(const unsigned int index) {
	// Setup the selectionfinder to search for the index string
	gtkutil::TreeModel::SelectionFinder finder(index, EFFECT_INDEX_COL);

	gtk_tree_model_foreach(
		gtk_tree_view_get_model(GTK_TREE_VIEW(_effectWidgets.view)),
		gtkutil::TreeModel::SelectionFinder::forEach,
		&finder
	);
	
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		// Set the active row of the list to the given effect
		gtk_tree_selection_select_iter(_effectWidgets.selection, &iter);
	}
}

void StimResponseEditor::moveEffect(int direction) {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Move the index (swap the specified indices)
			sr.moveEffect(effectIndex, effectIndex + direction);
			update();
			// Select the moved effect after the update
			selectEffectIndex(effectIndex + direction);
		}
	}
}

int StimResponseEditor::getEffectIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_effectWidgets.selection, &model, &iter
	);
	
	if (anythingSelected && _srEntity != NULL) {
		return gtkutil::TreeModel::getInt(model, &iter, EFFECT_INDEX_COL);
	}
	else {
		return -1;
	}
}

int StimResponseEditor::getIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_entitySRSelection, &model, &iter
	);
	
	if (anythingSelected && _srEntity != NULL) {
		return gtkutil::TreeModel::getInt(model, &iter, ID_COL);
	}
	else {
		return -1;
	}
}

void StimResponseEditor::save() {
	// Consistency check can go here
	
	// Save the working set to the entity
	_srEntity->save(_entity);
}

void StimResponseEditor::editEffect() {
	if (_srEntity == NULL) return;
	
	int id = getIdFromSelection();
	
	if (id > 0) {
		StimResponse& sr = _srEntity->get(id);
		int effectIndex = getEffectIdFromSelection();
		
		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0) {
			// Create a new effect editor (self-destructs)
			new EffectEditor(GTK_WINDOW(_dialog), sr, effectIndex, *this);
			
			// The editor is modal and will destroy itself, our work is done
		}
	}
}

void StimResponseEditor::updateEffectContextMenu() {
	// Check if we have anything selected at all
	int curEffectIndex = getEffectIdFromSelection();
	int highestEffectIndex = 0;
	
	bool anythingSelected = curEffectIndex >= 0;
	
	int srId = getIdFromSelection();
	if (srId > 0) {
		StimResponse& sr = _srEntity->get(srId);
		highestEffectIndex = sr.highestEffectIndex();
	}
	
	bool upActive = anythingSelected && curEffectIndex > 1;
	bool downActive = anythingSelected && curEffectIndex < highestEffectIndex;
	
	// Enable or disable the "Delete" context menu items based on the presence
	// of a selection.
	gtk_widget_set_sensitive(_effectWidgets.deleteMenuItem, anythingSelected);
		
	gtk_widget_set_sensitive(_effectWidgets.upMenuItem, upActive);
	gtk_widget_set_sensitive(_effectWidgets.downMenuItem, downActive);
}

// Static GTK Callbacks
gboolean StimResponseEditor::onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self) {
	// Toggle the visibility of the window
	self->toggle();
	
	// Don't propagate the delete event
	return TRUE;
}

// Callback for effects treeview selection changes
void StimResponseEditor::onEffectSelectionChange(GtkTreeSelection* selection, 
										   StimResponseEditor* self) 
{
	if (self->_updatesDisabled)	return; // Callback loop guard
	// Update the sensitivity
	self->updateEffectContextMenu();
}

void StimResponseEditor::onSave(GtkWidget* button, StimResponseEditor* self) {
	self->save();
}

void StimResponseEditor::onClose(GtkWidget* button, StimResponseEditor* self) {
	self->toggleWindow();
}

gboolean StimResponseEditor::onTreeViewKeyPress(
	GtkTreeView* view, GdkEventKey* event, StimResponseEditor* self)
{
	if (event->keyval == GDK_Delete) {
		if (view == GTK_TREE_VIEW(self->_entitySRView)) {
			//self->removeStimResponse();
		}
		else if (view == GTK_TREE_VIEW(self->_effectWidgets.view)) {
			self->removeEffect();
		}
		
		// Catch this keyevent, don't propagate
		return TRUE;
	}
	
	// Propagate further
	return FALSE;
}

gboolean StimResponseEditor::onWindowKeyPress(
	GtkWidget* dialog, GdkEventKey* event, StimResponseEditor* self)
{
	if (event->keyval == GDK_Escape) {
		self->toggleWindow();
		// Catch this keyevent, don't propagate
		return TRUE;
	}
	
	// Propagate further
	return FALSE;
}

// Button click events on TreeViews
gboolean StimResponseEditor::onTreeViewButtonPress(
	GtkTreeView* view, GdkEventButton* ev, StimResponseEditor* self)
{
	if (ev->type == GDK_2BUTTON_PRESS) {
		// Call the effect editor upon double click
		self->editEffect();
	}
	
	return FALSE;
}

// Button release event on TreeViews
gboolean StimResponseEditor::onTreeViewButtonRelease(
	GtkTreeView* view, GdkEventButton* ev, StimResponseEditor* self)
{
	// Single click with RMB (==> open context menu)
	if (ev->button == 3) {
		if (view == GTK_TREE_VIEW(self->_entitySRView))
			gtk_menu_popup(GTK_MENU(self->_stimListContextMenu), NULL, NULL, 
						   NULL, NULL, 1, GDK_CURRENT_TIME);
		else if (view == GTK_TREE_VIEW(self->_effectWidgets.view))
			gtk_menu_popup(GTK_MENU(self->_effectListContextMenu), NULL, NULL, 
						   NULL, NULL, 1, GDK_CURRENT_TIME);
	}
	
	return FALSE;
}

// Delete context menu items activated
void StimResponseEditor::_onContextMenuDelete(GtkWidget* w, 
											  StimResponseEditor* self)
{
	if (w == self->_srWidgets.deleteMenuItem) {
		//self->removeStimResponse();
	}
	else if (w == self->_effectWidgets.deleteMenuItem)
		self->removeEffect();
}

// Delete context menu items activated
void StimResponseEditor::_onContextMenuAdd(GtkWidget* w, 
										   StimResponseEditor* self)
{
	if (w == self->_srWidgets.addMenuItem) {
		//self->addStimResponse();
	}
	else if (w == self->_effectWidgets.addMenuItem) {
		self->addEffect();
	}
}

void StimResponseEditor::_onContextMenuEffectUp(GtkWidget* widget, 
	StimResponseEditor* self)
{
	self->moveEffect(-1);
}

void StimResponseEditor::_onContextMenuEffectDown(GtkWidget* widget, 
	StimResponseEditor* self)
{
	self->moveEffect(+1);
}

// Static command target
void StimResponseEditor::toggle() {
	Instance().toggleWindow();
}

StimResponseEditor& StimResponseEditor::Instance() {
	static StimResponseEditor _instance;
	return _instance;
}

} // namespace ui

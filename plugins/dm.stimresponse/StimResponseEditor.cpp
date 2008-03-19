#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "selectionlib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/dialog.h"
#include "string/string.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <iostream>

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Stim/Response Editor";
	
	const std::string RKEY_ROOT = "user/ui/stimResponseEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	
	const char* NO_ENTITY_ERROR = "A single entity must be selected to edit "
								  "Stim/Response properties."; 
}

StimResponseEditor::StimResponseEditor() :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow()),
	_entity(NULL),
	_stimEditor(_stimTypes),
	_responseEditor(getWindow(), _stimTypes),
	_customStimEditor(getWindow(), _stimTypes)
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
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();

	// Show the dialog, this enters the gtk main loop
	show();
}

void StimResponseEditor::_preHide() {
	_lastShownPage = gtk_notebook_get_current_page(_notebook);
	
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
}

void StimResponseEditor::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
	// Reload all the stim types, the map might have changed 
	_stimTypes.reload();
	// Scan the selection for entities
	rescanSelection();
		
	// Has the rescan found an entity (the pointer is non-NULL then)
	if (_entity != NULL) {
		// Now show the dialog window again
		gtk_widget_show_all(getWindow());
		// Show the last shown page
		gtk_notebook_set_current_page(_notebook, _lastShownPage);
	}
}

void StimResponseEditor::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), _dialogVBox);
	
	// Create the notebook and add it to the vbox
	_notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_box_pack_start(GTK_BOX(_dialogVBox), GTK_WIDGET(_notebook), TRUE, TRUE, 0);
	
	// The tab label items (icon + label)
	GtkWidget* stimLabelHBox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(
    	GTK_BOX(stimLabelHBox), 
    	gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(ICON_STIM + SUFFIX_EXTENSION)), 
    	FALSE, FALSE, 3
    );
	gtk_box_pack_start(GTK_BOX(stimLabelHBox), gtk_label_new("Stims"), FALSE, FALSE, 3);
	
	GtkWidget* responseLabelHBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(
    	GTK_BOX(responseLabelHBox), 
    	gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(ICON_RESPONSE + SUFFIX_EXTENSION)), 
    	FALSE, FALSE, 0
    );
	gtk_box_pack_start(GTK_BOX(responseLabelHBox), gtk_label_new("Responses"), FALSE, FALSE, 3);
	
	GtkWidget* customLabelHBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(
    	GTK_BOX(customLabelHBox), 
    	gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(ICON_CUSTOM_STIM)), 
    	FALSE, FALSE, 0
    );
	gtk_box_pack_start(GTK_BOX(customLabelHBox), gtk_label_new("Custom Stims"), FALSE, FALSE, 3);
	
	// Show the widgets before using them as label, they won't appear otherwise
	gtk_widget_show_all(stimLabelHBox);
	gtk_widget_show_all(responseLabelHBox);
	gtk_widget_show_all(customLabelHBox);
	
	// Cast the helper class to a widget and add it to the notebook page
	_stimPageNum = gtk_notebook_append_page(_notebook, _stimEditor, stimLabelHBox);
	_responsePageNum = gtk_notebook_append_page(_notebook, _responseEditor, responseLabelHBox);
	_customStimPageNum = gtk_notebook_append_page(_notebook, _customStimEditor, customLabelHBox);

	if (_lastShownPage == -1) {
		_lastShownPage = _stimPageNum;
	}
	
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

void StimResponseEditor::rescanSelection() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = NULL;
	_srEntity = SREntityPtr();
	_stimEditor.setEntity(_srEntity);
	_responseEditor.setEntity(_srEntity);
	_customStimEditor.setEntity(_srEntity);
	
	if (info.entityCount == 1 && info.totalCount == 1) {
		// Get the entity instance
		const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();
		
		_entity = Node_getEntity(node);
		
		_srEntity = SREntityPtr(new SREntity(_entity, _stimTypes));
		_stimEditor.setEntity(_srEntity);
		_responseEditor.setEntity(_srEntity);
		_customStimEditor.setEntity(_srEntity);
	}
	
	if (_entity != NULL) {
		std::string title = WINDOW_TITLE + " (" + _entity->getKeyValue("name") + ")";
		gtk_window_set_title(GTK_WINDOW(getWindow()), title.c_str()); 
	}
	else {
		gtk_window_set_title(GTK_WINDOW(getWindow()), WINDOW_TITLE.c_str());
	}
	
	gtk_widget_set_sensitive(_dialogVBox, _entity != NULL);
	gtk_widget_set_sensitive(_closeButton, TRUE);
}

void StimResponseEditor::save() {
	// Consistency check can go here
	
	// Scoped undo object
	UndoableCommand command("editStimResponse");
	
	// Save the working set to the entity
	_srEntity->save(_entity);
	
	// Save the custom stim types to the storage entity
	_stimTypes.save();
}

void StimResponseEditor::onSave(GtkWidget* button, StimResponseEditor* self) {
	self->save();
	self->destroy();
}

void StimResponseEditor::onClose(GtkWidget* button, StimResponseEditor* self) {
	self->destroy();
}

gboolean StimResponseEditor::onWindowKeyPress(
	GtkWidget* dialog, GdkEventKey* event, StimResponseEditor* self)
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
void StimResponseEditor::showDialog() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == 1) {
		// Construct a new instance, this enters the main loop
		StimResponseEditor _editor;
	}
	else {
		// Exactly one entity must be selected.
		gtkutil::errorDialog(NO_ENTITY_ERROR, GlobalRadiant().getMainWindow());
	}
}

int StimResponseEditor::_lastShownPage = -1;

} // namespace ui

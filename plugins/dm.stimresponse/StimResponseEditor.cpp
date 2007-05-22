#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iundo.h"
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
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog(gtkutil::TransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow(), false)),
	_entity(NULL),
	_stimEditor(_stimTypes),
	_responseEditor(_dialog, _stimTypes),
	_customStimEditor(_dialog, _stimTypes)
{
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

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
	
	instantiated = true;
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
		_lastShownPage = gtk_notebook_get_current_page(_notebook);
		// Save the window position, to make sure
		_windowPosition.readPosition();
		gtk_widget_hide_all(_dialog);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Reload all the stim types, the map might have changed 
		_stimTypes.reload();
		// Scan the selection for entities
		rescanSelection();
			
		// Has the rescan found an entity (the pointer is non-NULL then)
		if (_entity != NULL) {
			// Now show the dialog window again
			gtk_widget_show_all(_dialog);
			// Show the last shown page
			gtk_notebook_set_current_page(_notebook, _lastShownPage);
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
	_lastShownPage = _stimPageNum;
	
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
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& node = instance.path().top();
		
		_entity = Node_getEntity(node);
		
		_srEntity = SREntityPtr(new SREntity(_entity, _stimTypes));
		_stimEditor.setEntity(_srEntity);
		_responseEditor.setEntity(_srEntity);
		_customStimEditor.setEntity(_srEntity);
	}
	
	if (_entity != NULL) {
		std::string title = WINDOW_TITLE + " (" + _entity->getKeyValue("name") + ")";
		gtk_window_set_title(GTK_WINDOW(_dialog), title.c_str()); 
	}
	else {
		gtk_window_set_title(GTK_WINDOW(_dialog), WINDOW_TITLE.c_str());
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

// Static GTK Callbacks
gboolean StimResponseEditor::onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self) {
	// Toggle the visibility of the window
	self->toggle();
	
	// Don't propagate the delete event
	return TRUE;
}

void StimResponseEditor::onSave(GtkWidget* button, StimResponseEditor* self) {
	self->save();
	self->toggleWindow();
}

void StimResponseEditor::onClose(GtkWidget* button, StimResponseEditor* self) {
	self->toggleWindow();
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

// Static command target
void StimResponseEditor::toggle() {
	Instance().toggleWindow();
}

StimResponseEditor& StimResponseEditor::Instance() {
	static StimResponseEditor _instance;
	return _instance;
}

// Initialise the static boolean
bool ui::StimResponseEditor::instantiated = false;

} // namespace ui

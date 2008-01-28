#include "DifficultyEditor.h"

#include "iregistry.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "string/string.h"
#include "gtkutil/RightAlignment.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <iostream>

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Difficulty Editor";
	
	const std::string RKEY_ROOT = "user/ui/difficultyEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

DifficultyEditor::DifficultyEditor() :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow())
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

void DifficultyEditor::_preHide() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
}

void DifficultyEditor::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
}

void DifficultyEditor::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), _dialogVBox);
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
}

// Lower dialog buttons
GtkWidget* DifficultyEditor::createButtons() {

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

void DifficultyEditor::save() {
	// Consistency check can go here
	
	// Scoped undo object
	UndoableCommand command("editDifficulty");
	
	// Save the working set to the entity
	// TODO
}

void DifficultyEditor::onSave(GtkWidget* button, DifficultyEditor* self) {
	self->save();
	self->destroy();
}

void DifficultyEditor::onClose(GtkWidget* button, DifficultyEditor* self) {
	self->destroy();
}

gboolean DifficultyEditor::onWindowKeyPress(
	GtkWidget* dialog, GdkEventKey* event, DifficultyEditor* self)
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
void DifficultyEditor::showDialog() {
	// Construct a new instance, this enters the main loop
	DifficultyEditor _editor;
}

} // namespace ui

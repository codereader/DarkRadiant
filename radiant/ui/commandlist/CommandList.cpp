#include "CommandList.h"

#include "gtk/gtkliststore.h"
#include "gtk/gtktreeview.h"
#include "gtk/gtkhbox.h"
#include "gtk/gtkvbox.h"

#include "gtk/gtkdialog.h"
#include "gtk/gtklabel.h"
#include "gtk/gtkentry.h"
#include "gtk/gtkstock.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextButton.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"

#include "mainframe.h"
#include "CommandListPopulator.h"

namespace ui {

CommandListDialog::CommandListDialog() :
	DialogWindow(CMDLISTDLG_WINDOW_TITLE, MainFrame_getWindow()),
	_keyval(0),
	_state(0)
{
	setWindowSize(CMDLISTDLG_DEFAULT_SIZE_X, CMDLISTDLG_DEFAULT_SIZE_Y);
	
	// Create all the widgets
	populateWindow();
	
	// Show the window and its children
	gtk_widget_show_all(_window);
}

void CommandListDialog::reloadList() {
	gtk_list_store_clear(_listStore);
	
	// Instantiate the visitor class with the target list store
	CommandListPopulator populator(_listStore);

	// Cycle through all the events and create the according list items
	GlobalEventManager().foreachEvent(populator);
}

void CommandListDialog::populateWindow() {
	GtkHBox* hbox = GTK_HBOX(gtk_hbox_new(FALSE, 4));
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_container_add(GTK_CONTAINER(_window), GTK_WIDGET(hbox));

	{
		// Create a new liststore item and define its columns
		_listStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

		_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
		
		gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), gtkutil::TextColumn("Command", 0));
		gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), gtkutil::TextColumn("Key", 1));
		
		// Connect the mouseclick event to catch the double clicks
		g_signal_connect(G_OBJECT(_treeView), "button-press-event", G_CALLBACK(callbackViewButtonPress), this);
		
		gtk_widget_show(_treeView);

		// Load the list items into the treeview
		reloadList();

		g_object_unref(G_OBJECT(_listStore));
		
		// Pack this treeview into a scrolled window and show it
		GtkWidget* scrolled = gtkutil::ScrolledFrame(_treeView);
		gtk_widget_show_all(scrolled);
		
		// Pack the scrolled window into the hbox
		gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
	}

	GtkVBox* vbox = GTK_VBOX(gtk_vbox_new(FALSE, 4));
	gtk_widget_show(GTK_WIDGET(vbox));
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), FALSE, FALSE, 0);
	
	// Create the close button 
	{
		GtkWidget* closeButton = gtkutil::TextButton("Close");
		gtk_box_pack_end(GTK_BOX(vbox), closeButton, FALSE, FALSE, 4);
		g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(callbackClose), this);
	}
	// Create the assign shortcut button 
	{
		GtkWidget* assignButton = gtkutil::TextButton("Assign Shortcut");
		gtk_box_pack_end(GTK_BOX(vbox), assignButton, FALSE, FALSE, 4);
		g_signal_connect(G_OBJECT(assignButton), "clicked", G_CALLBACK(callbackAssign), this);
	}
	// Create the clear shortcut button 
	{
		GtkWidget* assignButton = gtkutil::TextButton("Clear Shortcut");
		gtk_box_pack_end(GTK_BOX(vbox), assignButton, FALSE, FALSE, 4);
		g_signal_connect(G_OBJECT(assignButton), "clicked", G_CALLBACK(callbackClear), this);
	}
}

bool CommandListDialog::shortcutDialog(const std::string& title, const std::string& label) {
	GtkWidget* dialog;
	GtkWidget* labelWidget;
	GtkWidget* entryWidget;
    
  	dialog = gtk_dialog_new_with_buttons(title.c_str(), GTK_WINDOW(_window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT, 
                                         GTK_STOCK_OK, GTK_RESPONSE_OK,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

	labelWidget = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelWidget), label.c_str());
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), labelWidget);
	
	entryWidget = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entryWidget);
	
	// The widget to display the status text
	_statusWidget = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), _statusWidget);
	gtk_label_set_justify(GTK_LABEL(_statusWidget), GTK_JUSTIFY_LEFT);
	
	// Connect the key events to the custom callback to override the default handler
	g_signal_connect(G_OBJECT(entryWidget), "key-press-event", G_CALLBACK(onShortcutKeyPress), this);
	
	gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	
	gtk_widget_destroy(dialog);
	
	return (response == GTK_RESPONSE_OK);
}

gboolean CommandListDialog::onShortcutKeyPress(GtkWidget* widget, GdkEventKey* event, CommandListDialog* self) {
	std::string statusText("");

	gtk_entry_set_text(GTK_ENTRY(widget), GlobalEventManager().getGDKEventStr(event).c_str());
	
	// Store this key/modifier combination for later use
	self->_keyval = event->keyval;
	self->_state = event->state;
	
	IEventPtr foundEvent = GlobalEventManager().findEvent(event);
	if (!foundEvent->empty()) {
		statusText = "Note: This is already assigned to: <b>";
		statusText += GlobalEventManager().getEventName(foundEvent) + "</b>";
	}
	
	gtk_label_set_markup(GTK_LABEL(self->_statusWidget), statusText.c_str());
	
	return true;
}

void CommandListDialog::retrieveShortcut(const std::string& commandName) {
	
	// The shortcutDialog returns TRUE, if the user clicked on OK
	if (shortcutDialog("Enter new Shortcut", std::string("<b>") + commandName + "</b>")) {
		if (_keyval != 0) {
			IEventPtr event = GlobalEventManager().findEvent(commandName);
			
			// Construct an eventkey structure to be passed to the EventManager query
			GdkEventKey eventKey;
			
			eventKey.keyval = _keyval; 
			eventKey.state = _state;
			
			IEventPtr foundEvent = GlobalEventManager().findEvent(&eventKey);
			
			// There is already a command connected to this shortcut, ask the user
			if (!foundEvent->empty()) {
				const std::string foundEventName = GlobalEventManager().getEventName(foundEvent);
				std::string message("The specified shortcut is already assigned to <b>");
				message += foundEventName + "</b>\nOverwrite the current setting and assign this shortcut to <b>";
				message += commandName + "</b> instead?";
				
				EMessageBoxReturn result = gtk_MessageBox(GTK_WIDGET(_window), message.c_str(), "Overwrite existing shortcut?", eMB_YESNO, eMB_ICONQUESTION);
				if (result == eIDYES) {
					// Disconnect both the found command and the new command
					GlobalEventManager().disconnectAccelerator(foundEventName);
					GlobalEventManager().disconnectAccelerator(commandName);
					
					// Create a new accelerator and connect it to the selected command
					IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
					GlobalEventManager().connectAccelerator(accel, commandName);
				}
			}
			else {
				GlobalEventManager().disconnectAccelerator(commandName);
				
				// Create a new accelerator and connect it to the selected command
				IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
				GlobalEventManager().connectAccelerator(accel, commandName);
			}
		}
		else {
			// No key is specified, disconnect the command
			GlobalEventManager().disconnectAccelerator(commandName);
		}
		
		// Update the list
		reloadList();
	}
}

std::string CommandListDialog::getSelectedCommand() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		const std::string commandName = gtkutil::TreeModel::getString(model, &iter, 0);
	
		IEventPtr event = GlobalEventManager().findEvent(commandName);
		
		// Double check, if the command exists
		if (event != NULL) {
			return commandName;
		}
	}

	return "";
}

void CommandListDialog::assignShortcut() {
	retrieveShortcut(getSelectedCommand());
}

void CommandListDialog::callbackAssign(GtkWidget* widget, CommandListDialog* self) {
	self->assignShortcut();
}

gboolean CommandListDialog::callbackViewButtonPress(GtkWidget* widget, GdkEventButton* event, CommandListDialog* self) {
	if (event->type == GDK_2BUTTON_PRESS) {
		self->assignShortcut();
	}
	
	return false;
}  

void CommandListDialog::callbackClear(GtkWidget* widget, CommandListDialog* self) {
	const std::string commandName = self->getSelectedCommand();
	
	if (commandName != "") {
		// Disconnect the event and update the list
		GlobalEventManager().disconnectAccelerator(commandName);
		self->reloadList();
	}
}

void CommandListDialog::callbackClose(GtkWidget* widget, CommandListDialog* self) {
	// Call the DialogWindow::destroy method and remove self from heap
	self->destroy();
}

} // namespace ui

void ShowCommandListDialog() {
	new ui::CommandListDialog(); // self-destructs in GTK callback
}

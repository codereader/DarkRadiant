#include "ShortcutChooser.h"

#include <gtk/gtkdialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkwidget.h>
#include <gdk/gdkkeysyms.h>

#include "gtkutil/messagebox.h"

namespace ui {

ShortcutChooser::ShortcutChooser(GtkWidget* parent) :
	_keyval(0),
	_state(0),
	_parent(parent)
{}

bool ShortcutChooser::shortcutDialog(const std::string& title, const std::string& label) {
	GtkWidget* dialog;
	GtkWidget* labelWidget;
	GtkWidget* entryWidget;
    
  	dialog = gtk_dialog_new_with_buttons(title.c_str(), GTK_WINDOW(_parent),
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

gboolean ShortcutChooser::onShortcutKeyPress(GtkWidget* widget, GdkEventKey* event, ShortcutChooser* self) {
	std::string statusText("");

	/** greebo: Workaround for allowing Shift+TAB as well (Tab becomes ISO_Left_Tab in that case)
	 */
	if (event->keyval == GDK_ISO_Left_Tab) {
		event->keyval = GDK_Tab;
	}

	// Store the shortcut string representation into the GtkEntry field  
	gtk_entry_set_text(GTK_ENTRY(widget), GlobalEventManager().getGDKEventStr(event).c_str());
	
	// Store this key/modifier combination for later use (UPPERCASE!)
	self->_keyval = gdk_keyval_to_upper(event->keyval);
	self->_state = event->state;
	
	IEventPtr foundEvent = GlobalEventManager().findEvent(event);
	
	// Only display the note if any event was found and it's not the "self" event
	if (!foundEvent->empty() && foundEvent != self->_event) {
		statusText = "Note: This is already assigned to: <b>";
		statusText += GlobalEventManager().getEventName(foundEvent) + "</b>";
	}
	
	gtk_label_set_markup(GTK_LABEL(self->_statusWidget), statusText.c_str());
	
	return true;
}

bool ShortcutChooser::retrieveShortcut(const std::string& commandName) {
	
	// The return value that states if the shortcuts have been touched or not
	bool shortcutsChanged = false;
	
	// Store the event pointer locally for later comparison in the GTK callback
	_event = GlobalEventManager().findEvent(commandName);
	
	// The shortcutDialog returns TRUE, if the user clicked on OK
	if (shortcutDialog("Enter new Shortcut", std::string("<b>") + commandName + "</b>")) {
		
		// Check, if the user has pressed a meaningful key
		if (_keyval != 0) {
			// Construct an eventkey structure to be passed to the EventManager query
			GdkEventKey eventKey;
			
			eventKey.keyval = _keyval; 
			eventKey.state = _state;
			
			// Try to lookup an existing command with the same shortcut
			IEventPtr foundEvent = GlobalEventManager().findEvent(&eventKey);
			
			// Only react on non-empty and non-"self" events
			if (!foundEvent->empty() && foundEvent != _event) {
				// There is already a command connected to this shortcut, ask the user
				const std::string foundEventName = GlobalEventManager().getEventName(foundEvent);
				
				// Construct the message
				std::string message("The specified shortcut is already assigned to <b>");
				message += foundEventName + "</b>\nOverwrite the current setting and assign this shortcut to <b>";
				message += commandName + "</b> instead?";
				
				// Fire up the dialog to ask the user what action to take
				EMessageBoxReturn result = gtk_MessageBox(GTK_WIDGET(_parent), 
														  message.c_str(), 
														  "Overwrite existing shortcut?", 
														  eMB_YESNO, eMB_ICONQUESTION);
				
				// Only react on "YES"
				if (result == eIDYES) {
					// Disconnect both the found command and the new command
					GlobalEventManager().disconnectAccelerator(foundEventName);
					GlobalEventManager().disconnectAccelerator(commandName);
					
					// Create a new accelerator and connect it to the selected command
					IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
					GlobalEventManager().connectAccelerator(accel, commandName);
					
					shortcutsChanged = true;
				}
			}
			else {
				// No command is using the accelerator up to now, so assign it
				
				// Disconnect the current command to avoid duplicate accelerators
				GlobalEventManager().disconnectAccelerator(commandName);
				
				// Create a new accelerator and connect it to the selected command
				IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
				GlobalEventManager().connectAccelerator(accel, commandName);
				
				shortcutsChanged = true;
			}
		}
		else {
			// No key is specified, disconnect the command
			GlobalEventManager().disconnectAccelerator(commandName);
			
			shortcutsChanged = true;
		}
	}
	
	return shortcutsChanged;
}

} // namespace ui

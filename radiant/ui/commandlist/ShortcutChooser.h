#ifndef SHORTCUTCHOOSER_H_
#define SHORTCUTCHOOSER_H_

#include <string>
#include "ieventmanager.h"

#include "gtk/gtkwidget.h"

typedef struct _GdkEventKey GdkEventKey;

/* greebo: The Shortcutchooser takes care of displaying the dialog and 
 * re-assigning the events after the shortcut has been entered by the user. 
 */

namespace ui {

class ShortcutChooser
{
	// The label to hold the status text of the shortcut chooser
	GtkWidget* _statusWidget;
	
	// Working variables to store the new key/modifier from the user input
	unsigned int _keyval;
	unsigned int _state;
	
	// The parent widget of the displayed dialog
	GtkWidget* _parent;
	
	// The event name the shortcut will be assigned to
	IEventPtr _event;

public:
	// Constructor, instantiate this class by specifying the parent window 
	ShortcutChooser(GtkWidget* parent);
	
	/* greebo: Asks the user to enter a new shortcut
	 * 
	 * @returns: TRUE, if the shortcut config has been changed, FALSE otherwise  
	 */
	bool retrieveShortcut(const std::string& commandName);

private:
	// Create the actual dialog and return a string with the result
	bool shortcutDialog(const std::string& title, const std::string& label);

	// The callback for catching the keypress events in the shortcut entry field
	static gboolean onShortcutKeyPress(GtkWidget* widget, GdkEventKey* event, ShortcutChooser* self);

}; // class ShortcutChooser

} // namespace ui

#endif /*SHORTCUTCHOOSER_H_*/

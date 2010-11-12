#ifndef SHORTCUTCHOOSER_H_
#define SHORTCUTCHOOSER_H_

#include <string>
#include "ieventmanager.h"
#include "gtkutil/dialog/Dialog.h"

typedef struct _GdkEventKey GdkEventKey;

namespace Gtk
{
	class Label;
	class Entry;
}

/**
 * greebo: The Shortcutchooser takes care of displaying the dialog and
 * re-assigning the events after the shortcut has been entered by the user.
 */
namespace ui
{

class ShortcutChooser :
	public gtkutil::Dialog
{
private:
	// The label to hold the status text of the shortcut chooser
	Gtk::Label* _statusWidget;
	Gtk::Entry* _entry;

	// Working variables to store the new key/modifier from the user input
	unsigned int _keyval;
	unsigned int _state;

	// The event name the shortcut will be assigned to
	std::string _commandName;
	IEventPtr _event;

public:
	// Constructor, instantiate this class by specifying the parent window
	ShortcutChooser(const std::string& title,
					const Glib::RefPtr<Gtk::Window>& parent,
					const std::string& command);

	// Override dialog::run
	ui::IDialog::Result run();

private:
	// Assigns or unassigns shortcut, based on the user's input (called after OK)
	// returns true if the shortcut actually got changed.
	bool assignShortcut();

	// The callback for catching the keypress events in the shortcut entry field
	bool onShortcutKeyPress(GdkEventKey* ev);

}; // class ShortcutChooser

} // namespace ui

#endif /*SHORTCUTCHOOSER_H_*/

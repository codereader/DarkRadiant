#include "ShortcutChooser.h"

#include "imainframe.h"
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gdk/gdkkeysyms.h>

#include "gtkutil/LeftAlignedLabel.h"
#include "i18n.h"
#include "idialogmanager.h"
#include <boost/format.hpp>

namespace ui
{

ShortcutChooser::ShortcutChooser(const std::string& title,
								 const Glib::RefPtr<Gtk::Window>& parent,
								 const std::string& command) :
	gtkutil::Dialog(title, GlobalMainFrame().getTopLevelWindow()),
	_statusWidget(NULL),
	_entry(NULL),
	_keyval(0),
	_state(0),
	_commandName(command),
	_event(GlobalEventManager().findEvent(_commandName))
{
	Gtk::Label* label = Gtk::manage(new Gtk::Label);
	label->set_markup("<b>" + _commandName + "</b>");
	_vbox->pack_start(*label, false, false, 0);

	_entry = Gtk::manage(new Gtk::Entry);
	_entry->signal_key_press_event().connect(sigc::mem_fun(*this, &ShortcutChooser::onShortcutKeyPress), false); // connect first
	_vbox->pack_start(*_entry, false, false, 0);

	// The widget to display the status text
	_statusWidget = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	_vbox->pack_start(*_statusWidget, false, false, 0);
}

bool ShortcutChooser::onShortcutKeyPress(GdkEventKey* ev)
{
	std::string statusText("");

	/** greebo: Workaround for allowing Shift+TAB as well (Tab becomes ISO_Left_Tab in that case)
	 */
	if (ev->keyval == GDK_ISO_Left_Tab)
	{
		ev->keyval = GDK_Tab;
	}

	// Store the shortcut string representation into the Entry field
	_entry->set_text(GlobalEventManager().getGDKEventStr(ev));

	// Store this key/modifier combination for later use (UPPERCASE!)
	_keyval = gdk_keyval_to_upper(ev->keyval);
	_state = ev->state;

	IEventPtr foundEvent = GlobalEventManager().findEvent(ev);

	// Only display the note if any event was found and it's not the "self" event
	if (!foundEvent->empty() && foundEvent != _event)
	{
		statusText = (boost::format(_("Note: This is already assigned to: <b>%s</b>")) %
					  GlobalEventManager().getEventName(foundEvent)).str();
	}

	_statusWidget->set_markup(statusText);

	return true; // don't propagate
}

ui::IDialog::Result ShortcutChooser::run()
{
	// Call base class
	Result result = gtkutil::Dialog::run();

	if (result == RESULT_OK)
	{
		assignShortcut();
	}

	return result;
}

bool ShortcutChooser::assignShortcut()
{
	bool shortcutsChanged = false;

	// Check, if the user has pressed a meaningful key
	if (_keyval != 0)
	{
		// Construct an eventkey structure to be passed to the EventManager query
		GdkEventKey eventKey;

		eventKey.keyval = _keyval;
		eventKey.state = _state;

		// Try to lookup an existing command with the same shortcut
		IEventPtr foundEvent = GlobalEventManager().findEvent(&eventKey);

		// Only react on non-empty and non-"self" events
		if (!foundEvent->empty() && foundEvent != _event)
		{
			// There is already a command connected to this shortcut, ask the user
			const std::string foundEventName = GlobalEventManager().getEventName(foundEvent);

			// Construct the message
			std::string message =
				(boost::format(_("The specified shortcut is already assigned to <b>%s</b>"
				"\nOverwrite the current setting and assign this shortcut to <b>%s</b> instead?")) %
				foundEventName % _commandName).str();

			// Fire up the dialog to ask the user what action to take
			IDialogPtr popup = GlobalDialogManager().createMessageBox(
				_("Overwrite existing shortcut?"), message, ui::IDialog::MESSAGE_ASK);

			// Only react on "YES"
			if (popup->run() == ui::IDialog::RESULT_YES)
			{
				// Disconnect both the found command and the new command
				GlobalEventManager().disconnectAccelerator(foundEventName);
				GlobalEventManager().disconnectAccelerator(_commandName);

				// Create a new accelerator and connect it to the selected command
				IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
				GlobalEventManager().connectAccelerator(accel, _commandName);

				shortcutsChanged = true;
			}
		}
		else
		{
			// No command is using the accelerator up to now, so assign it

			// Disconnect the current command to avoid duplicate accelerators
			GlobalEventManager().disconnectAccelerator(_commandName);

			// Create a new accelerator and connect it to the selected command
			IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
			GlobalEventManager().connectAccelerator(accel, _commandName);

			shortcutsChanged = true;
		}
	}
	else
	{
		// No key is specified, disconnect the command
		GlobalEventManager().disconnectAccelerator(_commandName);

		shortcutsChanged = true;
	}

	return shortcutsChanged;
}

} // namespace ui

#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "map/AutoSaver.h"
#include "imainframe.h"

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay) :
	TransientWindow(title, GlobalMainFrame().getTopLevelWindow()),
	_grabbedFocus(false)
{
	set_resizable(false);
	set_deletable(false);
	set_border_width(6);

	Gtk::Label* label = Gtk::manage(new gtkutil::LeftAlignedLabel(message));
	label->set_size_request(200, -1);

	add(*label);

	// Stop the autosaver
	map::AutoSaver().stopTimer();

	// Set the "screen updates disabled" flag
	GlobalMainFrame().disableScreenUpdates();

	// Connect the realize signal to remove the window decorations
	_realizeHandler = signal_realize().connect(
		sigc::mem_fun(*this, &ScreenUpdateBlocker::onRealize));

	if (GlobalMainFrame().isActiveApp() || forceDisplay)
	{
		// Show this window immediately
		show();

		// Eat all window events
		add_modal_grab();

		_grabbedFocus = true;

		// Process pending events to fully show the dialog
		while (Gtk::Main::events_pending())
		{
			Gtk::Main::iteration();
		}
	}

	// Register for the "is-active" changed event, to display this dialog
	// as soon as Radiant is getting the focus again
	_focusHandler = GlobalMainFrame().getTopLevelWindow()->connect_property_changed_with_return(
		"is-active",
		sigc::mem_fun(*this, &ScreenUpdateBlocker::onMainWindowFocus)
	);
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	_realizeHandler.disconnect();
	_focusHandler.disconnect();

	// Process pending events to flush keystroke buffer etc.
	while (Gtk::Main::events_pending())
	{
		Gtk::Main::iteration();
	}

	// Remove the event blocker, if appropriate
	if (_grabbedFocus)
	{
		remove_modal_grab();
	}

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

	// Start the autosaver again
	map::AutoSaver().startTimer();

	destroy();
}

void ScreenUpdateBlocker::onMainWindowFocus()
{
	// The Radiant main window has changed its active state, let's see if it became active
	// and if yes, show this blocker dialog.
	if (GlobalMainFrame().getTopLevelWindow()->property_is_active() && !is_visible())
	{
		show();
	}
}

void ScreenUpdateBlocker::onRealize()
{
	// Disable some decorations
	get_window()->set_decorations(Gdk::DECOR_ALL|Gdk::DECOR_MENU|Gdk::DECOR_MINIMIZE|Gdk::DECOR_MAXIMIZE);
}

} // namespace ui

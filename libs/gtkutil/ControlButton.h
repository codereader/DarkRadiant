#ifndef CONTROLBUTTON_H_
#define CONTROLBUTTON_H_

#include "Timer.h"

#include <gtkmm/image.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>

namespace gtkutil {

	namespace
	{
		// The delay between the first "click" and the second "click" event
		const int DELAY_INITIAL = 200;
		// The delay between all following "click" events
		const int DELAY_PERIODIC = 20;
	}

/**
 * A button containing a single icon that keeps periodically emitting the
 * "clicked" event as long as the user keeps the mouse button pressed.
 * Used for Surface Inspector controls, for example.
 */
class ControlButton :
	public Gtk::Button
{
private:
	// The timer object that periodically fires the onTimeOut() method
	Timer _timer;

public:

	ControlButton(const Glib::RefPtr<Gdk::Pixbuf>& iconPixbuf) :
		Gtk::Button(),
		_timer(DELAY_INITIAL, onTimeOut, this)
	{
		_timer.disable();

		// Add the icon to the button
		Gtk::Image* icon = Gtk::manage(new Gtk::Image(iconPixbuf));

		Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 3));
		vbox->pack_start(*icon, true, false, 0);

		add(*vbox);

		// Connect the pressed/released signals
		signal_pressed().connect(sigc::mem_fun(*this, &ControlButton::onPress), false);
		signal_released().connect(sigc::mem_fun(*this, &ControlButton::onRelease), false);
	}

	static gboolean onTimeOut(gpointer data)
	{
		ControlButton* self = reinterpret_cast<ControlButton*>(data);

		// Fire the "clicked" signal
		self->clicked();

		// Set the interval to a smaller value
		self->_timer.setTimeout(DELAY_PERIODIC);
		self->_timer.enable();

		// Return true, so that the timer gets called again
		return TRUE;
	}

	void onPress()
	{
		// Connect the timing event
		_timer.enable();
	}

	void onRelease()
	{
		// Disconnect the timing event
		_timer.disable();
		// Reset the interval to the initial value
		_timer.setTimeout(DELAY_INITIAL);
	}
};

} // namespace gtkutil

#endif /*CONTROLBUTTON_H_*/

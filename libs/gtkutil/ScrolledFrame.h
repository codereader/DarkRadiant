#ifndef SCROLLEDFRAME_H_
#define SCROLLEDFRAME_H_

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/frame.h>

namespace gtkutil
{

/**
 * Container widget which packs its child into a GtkScrolledWindow.
 */
class ScrolledFrame :
	public Gtk::ScrolledWindow
{
public:
	/**
	 * Construct a ScrolledFrame around the provided child widget.
	 * @useViewPort: use this to add "non-scrollable" widgets to this container.
	 */
	ScrolledFrame(Gtk::Widget& child, bool useViewPort = false)
	{
		// Create the GtkScrolledWindow
		set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		set_shadow_type(Gtk::SHADOW_ETCHED_IN);

		add(child);
	}
};

}

#endif /*SCROLLEDFRAME_H_*/

#ifndef FRAMEDWIDGET_H_
#define FRAMEDWIDGET_H_

#include <gtkmm/frame.h>

namespace gtkutil
{

/**
 * greebo: Shortcut class to create a framed, contained widget.
 *
 * Pass the widget to be contained to the class constructor.
 */
class FramedWidget :
	public Gtk::Frame
{
protected:
	// The contained widget
	Gtk::Widget& _containedWidget;

public:
	// Constructor
	FramedWidget(Gtk::Widget& containedWidget) :
		_containedWidget(containedWidget)
	{
		show();
		set_shadow_type(Gtk::SHADOW_IN);

		// Add the contained widget as children to the frame
		add(_containedWidget);
		_containedWidget.show();

		// Now show the whole widget tree
		show_all();
	}
};

} // namespace gtkutil

#endif /*FRAMEDWIDGET_H_*/

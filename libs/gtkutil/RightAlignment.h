#ifndef RIGHTALIGNMENT_H_
#define RIGHTALIGNMENT_H_

#include <gtkmm/alignment.h>

namespace gtkutil
{

/** A GtkAlignment wrapper that right-aligns any child widget.
 */
class RightAlignment :
	public Gtk::Alignment
{
public:
	/** Constructor. Accepts the child widget to align.
	 */
	RightAlignment(Gtk::Widget& child) :
		Gtk::Alignment(1.0f, 0.0f, 0, 0)
	{
		add(child);
	}
};

}

#endif /*RIGHTALIGNMENT_H_*/

#ifndef LEFTALIGNMENT_H_
#define LEFTALIGNMENT_H_

#include <gtkmm/alignment.h>

namespace gtkutil
{

/**
 * \addtogroup gtkutil GTK utility library
 *
 * \namespace gtkutil
 * Helper classes and functions to achieve common GTK-related tasks, including
 * many useful compound widgets with C++-style interfaces.
 * \ingroup gtkutil
 */

/**
 * Container that left-aligns a child widget, with configurable indentation
 * and fill ratio.
 *
 * \ingroup gtkutil
 */
class LeftAlignment :
	public Gtk::Alignment
{
public:
	/**
	 * Construct a LeftAlignment displaying the given widget with the given
	 * indentation parameters.
	 *
	 * @param w
	 * The child widget.
	 *
	 * @param indent
	 * Number of pixels to left-indent by (default 0).
	 *
	 * @param expand
	 * Amount by which the child should expand, from 0.0 (no expansion, widget
	 * remains at default size) to 1.0 (widget expands to fill all available
	 * space).
	 */
	LeftAlignment(Gtk::Widget& child, int indent = 0, float expand = 0.0) :
		Gtk::Alignment(0.0, 0.5, expand, 1.0)
	{
		set_padding(0, 0, indent, 0);
		add(child);
	}
};

}

#endif /*LEFTALIGNMENT_H_*/

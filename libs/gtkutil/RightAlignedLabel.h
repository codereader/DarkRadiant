#ifndef RIGHTALIGNEDLABEL_H_
#define RIGHTALIGNEDLABEL_H_

#include <gtkmm/label.h>
#include "RightAlignment.h"

namespace gtkutil
{

/** A GtkLabel that is right-aligned, rather than the default centered align-
 * ment.
 */
class RightAlignedLabel :
	public Gtk::Label
{
public:
	/** Construct a right-aligned label with the given text.
	 */
	RightAlignedLabel(const std::string& text) :
		Gtk::Label()
	{
		set_markup(text);
		set_alignment(1.0f, 0.5f);
	}
};

}

#endif /*RIGHTALIGNEDLABEL_H_*/

#ifndef LEFTALIGNEDLABEL_H_
#define LEFTALIGNEDLABEL_H_

#include <string>
#include <gtkmm/label.h>

namespace gtkutil
{

/** A GtkLabel that is left-aligned, rather than the default centered align-
 * ment.
 */
class LeftAlignedLabel :
	public Gtk::Label
{
public:
	/** Construct a left-aligned label with the given text.
	 */
	LeftAlignedLabel(const std::string& text) :
		Gtk::Label()
	{
		set_markup(text);
		set_alignment(0.0f, 0.5f);
	}
};

}

#endif /*LEFTALIGNEDLABEL_H_*/

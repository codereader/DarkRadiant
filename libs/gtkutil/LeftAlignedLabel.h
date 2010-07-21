#ifndef LEFTALIGNEDLABEL_H_
#define LEFTALIGNEDLABEL_H_

#include <string>
#include <gtk/gtklabel.h>

#include <gtkmm/label.h>

namespace gtkutil
{

/** A GtkLabel that is left-aligned, rather than the default centered align-
 * ment.
 */

class LeftAlignedLabel
{
	// The label
	GtkWidget* _label;
	
public:

	/** Construct a left-aligned label with the given text.
	 */
	LeftAlignedLabel(const std::string& text)
	: _label(gtk_label_new(NULL))
	{ 
		gtk_label_set_markup(GTK_LABEL(_label), text.c_str());
	}
	
	/** Operator cast to GtkWidget*. Left-aligns then returns the GtkLabel.
	 */
	operator GtkWidget* () {
		gtk_misc_set_alignment(GTK_MISC(_label), 0.0, 0.5);
		return _label;
	}
};

// gtkmm pendant
class LeftAlignedLabelmm
{
private:
	// The label
	Gtk::Label* _label;
	
public:

	/** Construct a left-aligned label with the given text.
	 */
	LeftAlignedLabelmm(const std::string& text) : 
		_label(Gtk::manage(new Gtk::Label))
	{ 
		_label->set_text(text);
	}
	
	/** Operator cast to Gtk::Widget&. Left-aligns then returns the label.
	 */
	operator Gtk::Label*()
	{
		_label->set_alignment(0.0f, 0.5f);
		return _label;
	}
};

}

#endif /*LEFTALIGNEDLABEL_H_*/

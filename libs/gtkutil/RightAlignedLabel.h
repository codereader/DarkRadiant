#ifndef RIGHTALIGNEDLABEL_H_
#define RIGHTALIGNEDLABEL_H_

#include <gtk/gtklabel.h>
#include <gtkmm/label.h>
#include "RightAlignment.h"

namespace gtkutil
{

/** A GtkLabel that is right-aligned, rather than the default centered align-
 * ment.
 */

class RightAlignedLabel
{
	// The label
	GtkWidget* _label;
	
public:

	/** Construct a right-aligned label with the given text.
	 */
	RightAlignedLabel(const std::string& text)
	: _label(gtk_label_new(NULL))
	{ 
		gtk_label_set_markup(GTK_LABEL(_label), text.c_str());
	}
	
	/** Operator cast to GtkWidget*. Left-aligns then returns the GtkLabel.
	 */
	operator GtkWidget* () {
		GtkWidget* rightAlignedLabel = RightAlignment(_label);
		return rightAlignedLabel;
	}
};

// gtkmm pendant
class RightAlignedLabelmm :
	public Gtk::Label
{
public:
	/** Construct a right-aligned label with the given text.
	 */
	RightAlignedLabelmm(const std::string& text) :
		Gtk::Label()
	{ 
		set_markup(text);
		set_alignment(1.0f, 0.5f);
	}
};

}

#endif /*RIGHTALIGNEDLABEL_H_*/

#ifndef RIGHTALIGNEDLABEL_H_
#define RIGHTALIGNEDLABEL_H_

#include <gtk/gtklabel.h>
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

}

#endif /*RIGHTALIGNEDLABEL_H_*/

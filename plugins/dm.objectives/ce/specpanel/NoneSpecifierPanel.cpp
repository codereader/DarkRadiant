#include "NoneSpecifierPanel.h"

#include <gtk/gtklabel.h>

namespace objectives
{

namespace ce
{

// Registration helper
NoneSpecifierPanel::RegHelper NoneSpecifierPanel::_regHelper;

// Constructor
NoneSpecifierPanel::NoneSpecifierPanel()
{
	_widget = gtk_label_new("This specifier type does not take a value.");
}

// Destructor
NoneSpecifierPanel::~NoneSpecifierPanel() 
{
	if (GTK_IS_WIDGET(_widget))
		gtk_widget_destroy(_widget);
}

// Show and return the widget
GtkWidget* NoneSpecifierPanel::getWidget() const {
	gtk_widget_show_all(_widget);
	return _widget;
}

}

}

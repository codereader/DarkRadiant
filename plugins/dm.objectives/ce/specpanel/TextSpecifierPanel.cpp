#include "TextSpecifierPanel.h"

#include <gtk/gtkentry.h>

namespace objectives
{

namespace ce
{

// Constructor
TextSpecifierPanel::TextSpecifierPanel()
{
	_widget = gtk_entry_new();
}

// Destructor
TextSpecifierPanel::~TextSpecifierPanel() 
{
	if (GTK_IS_WIDGET(_widget))
		gtk_widget_destroy(_widget);
}

// Show and return the widget
GtkWidget* TextSpecifierPanel::getWidget() const {
	gtk_widget_show_all(_widget);
	return _widget;
}

}

}

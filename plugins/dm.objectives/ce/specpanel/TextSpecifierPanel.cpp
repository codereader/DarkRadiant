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
GtkWidget* TextSpecifierPanel::_getWidget() const {
	return _widget;
}

// Set the displayed value
void TextSpecifierPanel::setValue(const std::string& value)
{
    gtk_entry_set_text(GTK_ENTRY(_widget), value.c_str());
}

// Get the edited value
std::string TextSpecifierPanel::getValue() const
{
    return std::string(gtk_entry_get_text(GTK_ENTRY(_widget)));
}

} // namespace ce

} // namespace objectives

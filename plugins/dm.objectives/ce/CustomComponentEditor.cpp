#include "CustomComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
CustomComponentEditor::RegHelper CustomComponentEditor::regHelper;

// Constructor
CustomComponentEditor::CustomComponentEditor(Component& component) :
	_component(&component)
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Item:</b>"),
        FALSE, FALSE, 0
    );

	// TODO
}

// Destructor
CustomComponentEditor::~CustomComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* CustomComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void CustomComponentEditor::writeToComponent() const {
    assert(_component);
	// TODO
}

} // namespace ce

} // namespace objectives

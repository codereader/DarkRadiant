#include "CustomClockedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
CustomClockedComponentEditor::RegHelper CustomClockedComponentEditor::regHelper;

// Constructor
CustomClockedComponentEditor::CustomClockedComponentEditor(Component& component) :
	_component(&component),
	_scriptFunction(gtk_entry_new()),
	_interval(gtk_spin_button_new_with_range(0, 65535, 0.1))
{
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_interval), 2);

	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Script Function:</b>"),
        FALSE, FALSE, 0
    );
	gtk_box_pack_start(GTK_BOX(_widget), _scriptFunction, FALSE, FALSE, 0);

	gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Clock interval:</b>"),
        FALSE, FALSE, 0
    );

	GtkWidget* hbox2 = gtk_hbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(hbox2), _interval, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), gtkutil::LeftAlignedLabel("seconds"), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(_widget), hbox2, FALSE, FALSE, 0);

	// Load the initial values into the boxes
	gtk_entry_set_text(GTK_ENTRY(_scriptFunction), component.getArgument(0).c_str());

	float interval = component.getClockInterval();
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(_interval), interval >= 0 ? interval : 1.0);
}

// Destructor
CustomClockedComponentEditor::~CustomClockedComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* CustomClockedComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void CustomClockedComponentEditor::writeToComponent() const {
    assert(_component);
	
	_component->setArgument(0, gtk_entry_get_text(GTK_ENTRY(_scriptFunction)));
	_component->setClockInterval(
		static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_interval))));
}

} // namespace ce

} // namespace objectives

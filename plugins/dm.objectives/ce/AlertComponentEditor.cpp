#include "AlertComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/string.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper
AlertComponentEditor::RegHelper AlertComponentEditor::regHelper;

// Constructor
AlertComponentEditor::AlertComponentEditor(Component& component) : 
	_component(&component),
	_targetCombo(SpecifierType::SET_STANDARD_AI()),
	_amount(gtk_spin_button_new_with_range(0, 65535, 1)),
	_alertLevel(gtk_spin_button_new_with_range(1, 5, 1))
{
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_amount), 0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_alertLevel), 0);

	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>AI:</b>"),
        FALSE, FALSE, 0
    );

	// Create a new hbox for the targetcombo and the amount spin button
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), hbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), _targetCombo.getWidget(), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::LeftAlignedLabel("Amount:"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _amount, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::LeftAlignedLabel("Minimum Alert Level:"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _alertLevel, FALSE, FALSE, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _targetCombo.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin buttons with the values from the component arguments
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(_amount), 
		strToDouble(component.getArgument(0))
	);

	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(_alertLevel), 
		strToDouble(component.getArgument(1))
	);
}

// Destructor
AlertComponentEditor::~AlertComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* AlertComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void AlertComponentEditor::writeToComponent() const {
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _targetCombo.getSpecifier()
    );

	_component->setArgument(0, 
		doubleToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_amount))));

	_component->setArgument(1, 
		doubleToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_alertLevel))));
}

} // namespace ce

} // namespace objectives

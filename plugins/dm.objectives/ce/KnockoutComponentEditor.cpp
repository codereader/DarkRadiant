#include "KnockoutComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/string.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Registration helper
KnockoutComponentEditor::RegHelper KnockoutComponentEditor::regHelper;

// Constructor
KnockoutComponentEditor::KnockoutComponentEditor(Component& component)
: _component(&component),
  _targetCombo(SpecifierType::SET_STANDARD_AI())
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Knockout target:</b>"),
        FALSE, FALSE, 0
    );

	// Create a new hbox for the targetcombo and the amount spin button
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), hbox, FALSE, FALSE, 0);

	_amountSpinButton = gtk_spin_button_new_with_range(0, 65535, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_amountSpinButton), 0);

	gtk_box_pack_start(GTK_BOX(hbox), _targetCombo.getWidget(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::LeftAlignedLabel("Amount:"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _amountSpinButton, FALSE, FALSE, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _targetCombo.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(_amountSpinButton), 
		strToDouble(component.getArgument(0))
	);
}

// Destructor
KnockoutComponentEditor::~KnockoutComponentEditor() {
	if (GTK_IS_WIDGET(_widget))
		gtk_widget_destroy(_widget);
}

// Get the main widget
GtkWidget* KnockoutComponentEditor::getWidget() const
{
	return _widget;
}

// Write to component
void KnockoutComponentEditor::writeToComponent() const
{
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _targetCombo.getSpecifier()
    );

	_component->setArgument(0, 
		doubleToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_amountSpinButton)))); 
}

}

}

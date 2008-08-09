#include "PickpocketComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/string.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
PickpocketComponentEditor::RegHelper PickpocketComponentEditor::regHelper;

// Constructor
PickpocketComponentEditor::PickpocketComponentEditor(Component& component) :
	_component(&component),
	_itemSpec(SpecifierType::SET_ITEM()),
	_amount(gtk_spin_button_new_with_range(0, 65535, 1))
{
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_amount), 0);

	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignedLabel("<b>Item:</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), _itemSpec.getWidget(), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignedLabel("<b>Amount:</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignment(_amount), FALSE, FALSE, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _itemSpec.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(_amount), 
		strToDouble(component.getArgument(0))
	);
}

// Destructor
PickpocketComponentEditor::~PickpocketComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* PickpocketComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void PickpocketComponentEditor::writeToComponent() const {
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _itemSpec.getSpecifier()
    );

	_component->setArgument(0, 
		doubleToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_amount))));
}

} // namespace ce

} // namespace objectives

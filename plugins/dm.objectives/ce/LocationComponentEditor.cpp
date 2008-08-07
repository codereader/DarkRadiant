#include "LocationComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
LocationComponentEditor::RegHelper LocationComponentEditor::regHelper;

// Constructor
LocationComponentEditor::LocationComponentEditor(Component& component) :
	_component(&component),
	_entSpec(SpecifierType::SET_ITEM()),
	_locationSpec(SpecifierType::SET_ITEM())
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Item:</b>"),
        FALSE, FALSE, 0
    );
	gtk_box_pack_start(
		GTK_BOX(_widget), _entSpec.getWidget(), FALSE, FALSE, 0
	);
	gtk_box_pack_start(
		GTK_BOX(_widget), _locationSpec.getWidget(), FALSE, FALSE, 0
	);

    // Populate the SpecifierEditCombo with the first specifier
    _entSpec.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	_locationSpec.setSpecifier(
		component.getSpecifier(Specifier::SECOND_SPECIFIER)
    );
}

// Destructor
LocationComponentEditor::~LocationComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* LocationComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void LocationComponentEditor::writeToComponent() const {
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _entSpec.getSpecifier()
    );

	_component->setSpecifier(
		Specifier::SECOND_SPECIFIER, _locationSpec.getSpecifier()
    );
}

} // namespace ce

} // namespace objectives

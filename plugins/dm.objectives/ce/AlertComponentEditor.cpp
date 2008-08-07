#include "AlertComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper
AlertComponentEditor::RegHelper AlertComponentEditor::regHelper;

// Constructor
AlertComponentEditor::AlertComponentEditor(Component& component)
: _component(&component),
  _targetCombo(SpecifierType::SET_STANDARD_AI())
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Kill target:</b>"),
        FALSE, FALSE, 0
    );
	gtk_box_pack_start(
		GTK_BOX(_widget), _targetCombo.getWidget(), FALSE, FALSE, 0
	);

    // Populate the SpecifierEditCombo with the first specifier
    _targetCombo.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
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
}

} // namespace ce

} // namespace objectives

#include "ReadableOpenedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "i18n.h"
#include <gtk/gtk.h>
#include "string/string.h"

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
ReadableOpenedComponentEditor::RegHelper ReadableOpenedComponentEditor::regHelper;

// Constructor
ReadableOpenedComponentEditor::ReadableOpenedComponentEditor(Component& component) :
	_component(&component),
	_readableSpec(SpecifierType::SET_READABLE())
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignedLabel(std::string("<b>") + _("Readable:") + "</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), _readableSpec.getWidget(), TRUE, TRUE, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _readableSpec.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );
}

// Destructor
ReadableOpenedComponentEditor::~ReadableOpenedComponentEditor()
{
	if (GTK_IS_WIDGET(_widget))
	{
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* ReadableOpenedComponentEditor::getWidget() const
{
	return _widget;
}

// Write to component
void ReadableOpenedComponentEditor::writeToComponent() const
{
    assert(_component);

    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _readableSpec.getSpecifier()
    );
}

} // namespace ce

} // namespace objectives

#include "ReadablePageReachedComponentEditor.h"
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
ReadablePageReachedComponentEditor::RegHelper ReadablePageReachedComponentEditor::regHelper;

// Constructor
ReadablePageReachedComponentEditor::ReadablePageReachedComponentEditor(Component& component) :
	_component(&component),
	_readableSpec(SpecifierType::SET_READABLE()),
	_pageNum(gtk_spin_button_new_with_range(1, 65535, 1))
{
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_pageNum), 0);

	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignedLabel(std::string("<b>") + _("Readable:") + "</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), _readableSpec.getWidget(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignedLabel(_("Page Number:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), gtkutil::LeftAlignment(_pageNum), FALSE, FALSE, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _readableSpec.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(_pageNum), 
		strToDouble(component.getArgument(0))
	);
}

// Destructor
ReadablePageReachedComponentEditor::~ReadablePageReachedComponentEditor()
{
	if (GTK_IS_WIDGET(_widget))
	{
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* ReadablePageReachedComponentEditor::getWidget() const
{
	return _widget;
}

// Write to component
void ReadablePageReachedComponentEditor::writeToComponent() const
{
    assert(_component);

    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _readableSpec.getSpecifier()
    );

	_component->setArgument(0, 
		doubleToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_pageNum))));
}

} // namespace ce

} // namespace objectives

#include "DistanceComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/string.h"

#include "i18n.h"
#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
DistanceComponentEditor::RegHelper DistanceComponentEditor::regHelper;

// Constructor
DistanceComponentEditor::DistanceComponentEditor(Component& component) :
	_component(&component),
	_entity(gtk_entry_new()),
	_location(gtk_entry_new()),
	_distance(gtk_spin_button_new_with_range(0, 132000, 1)),
	_interval(gtk_spin_button_new_with_range(0, 65535, 0.1))
{
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_interval), 2);
	// Allow for one digit of the distance, everything below this step size is insane
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(_distance), 1);

	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), hbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), gtkutil::LeftAlignedLabel(_("Entity:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _entity, TRUE, TRUE, 0);

	GtkWidget* hbox2 = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), hbox2, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox2), gtkutil::LeftAlignedLabel(_("Location Entity:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), _location, TRUE, TRUE, 0);

	GtkWidget* hbox3 = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), hbox3, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox3), gtkutil::LeftAlignedLabel(_("Distance:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox3), _distance, TRUE, TRUE, 0);

	// The second row contains the clock interval
	GtkWidget* hbox4 = gtk_hbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(hbox4), gtkutil::LeftAlignedLabel(_("Clock interval:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox4), _interval, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox4), gtkutil::LeftAlignedLabel(_("seconds")), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(_widget), hbox4, FALSE, FALSE, 0);

	// Load the initial values from the component arguments
	gtk_entry_set_text(GTK_ENTRY(_entity), component.getArgument(0).c_str());
	gtk_entry_set_text(GTK_ENTRY(_location), component.getArgument(1).c_str());
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(_distance), 
		strToDouble(component.getArgument(2))
	);
	float interval = component.getClockInterval();
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(_interval), interval >= 0 ? interval : 1.0);
}

// Destructor
DistanceComponentEditor::~DistanceComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* DistanceComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void DistanceComponentEditor::writeToComponent() const {
    assert(_component);
    
	_component->setArgument(0, gtk_entry_get_text(GTK_ENTRY(_entity)));
	_component->setArgument(1, gtk_entry_get_text(GTK_ENTRY(_location)));
	_component->setArgument(2, doubleToStr(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_distance))));

	_component->setClockInterval(
		static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(_interval))));
}

} // namespace ce

} // namespace objectives

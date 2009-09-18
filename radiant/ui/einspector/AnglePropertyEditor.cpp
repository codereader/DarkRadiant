#include "AnglePropertyEditor.h"

#include "iradiant.h"
#include "ientity.h"
#include "string/string.h"

#include "gtkutil/IconTextButton.h"
#include "gtkutil/pointer.h"

#include <gtk/gtk.h>

namespace ui
{

	namespace
	{
		const char* const GLIB_ANGLE_KEY = "dr_angle_value";
	}

// Constructor
AnglePropertyEditor::AnglePropertyEditor(Entity* entity, const std::string& key)
: PropertyEditor(entity),
  _key(key)
{
    // Construct a 3x3 table to contain the directional buttons
    GtkWidget* table = gtk_table_new(3, 3, TRUE);

    // Create the buttons
    constructButtons();

    // Add buttons
    gtk_table_attach_defaults(GTK_TABLE(table), _nButton, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), _sButton, 1, 2, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), _eButton, 2, 3, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), _wButton, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), _neButton, 2, 3, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), _seButton, 2, 3, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), _swButton, 0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), _nwButton, 0, 1, 0, 1);

    // Pack table into an hbox/vbox and set as the widget
    GtkWidget* hbx = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbx), table, TRUE, FALSE, 0);

    _widget = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(_widget), hbx, FALSE, FALSE, 0);
}

GtkWidget* AnglePropertyEditor::constructAngleButton(
	const std::string& icon, int angleValue)
{
	GtkWidget* w = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf(icon), false
    );

    g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(_onButtonClick), this);
	g_object_set_data(G_OBJECT(w), GLIB_ANGLE_KEY, gpointer(angleValue));

	return w;
}

// Construct the buttons
void AnglePropertyEditor::constructButtons()
{
    _nButton = constructAngleButton("arrow_n24.png", 90);
    _neButton = constructAngleButton("arrow_ne24.png", 45);
	_eButton = constructAngleButton("arrow_e24.png", 0);
	_seButton = constructAngleButton("arrow_se24.png", 315);
	_sButton = constructAngleButton("arrow_s24.png", 270);
	_swButton = constructAngleButton("arrow_sw24.png", 225);
	_wButton = constructAngleButton("arrow_w24.png", 180);
	_nwButton = constructAngleButton("arrow_nw24.png", 135);
}

// GTK button callback
void AnglePropertyEditor::_onButtonClick(GtkButton* button,
                                         AnglePropertyEditor* self)
{
    // Get the numerical value off the button
	gint angle = gpointer_to_int(g_object_get_data(G_OBJECT(button), GLIB_ANGLE_KEY));

	self->setKeyValue(self->_key, intToStr(angle));
}

} // namespace ui

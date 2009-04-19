#include "AnglePropertyEditor.h"

#include "iradiant.h"
#include "ientity.h"

#include "gtkutil/IconTextButton.h"

#include <gtk/gtk.h>

namespace ui
{

// Constructor
AnglePropertyEditor::AnglePropertyEditor(Entity* entity, const std::string& key)
: _entity(entity),
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

// Construct the buttons
void AnglePropertyEditor::constructButtons()
{
    _nButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_n24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_nButton), "clicked", G_CALLBACK(_onButtonClick), this
    );

    _neButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_ne24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_neButton), "clicked", G_CALLBACK(_onButtonClick), this
    );
    
    _eButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_e24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_eButton), "clicked", G_CALLBACK(_onButtonClick), this
    );

    _seButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_se24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_seButton), "clicked", G_CALLBACK(_onButtonClick), this
    );

    _sButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_s24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_sButton), "clicked", G_CALLBACK(_onButtonClick), this
    );

    _swButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_sw24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_swButton), "clicked", G_CALLBACK(_onButtonClick), this
    );

    _wButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_w24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_wButton), "clicked", G_CALLBACK(_onButtonClick), this
    );

    _nwButton = gtkutil::IconTextButton(
        "", GlobalRadiant().getLocalPixbuf("arrow_nw24.png"), false
    );
    g_signal_connect(
        G_OBJECT(_nwButton), "clicked", G_CALLBACK(_onButtonClick), this
    );
}

// GTK button callback
void AnglePropertyEditor::_onButtonClick(GtkButton* button,
                                         AnglePropertyEditor* self)
{
    // Set the numerical value based on which button was clicked
    if (button == GTK_BUTTON(self->_nButton))
    {
        self->_entity->setKeyValue(self->_key, "90");
    }
    else if (button == GTK_BUTTON(self->_neButton))
    {
        self->_entity->setKeyValue(self->_key, "45");
    }
    else if (button == GTK_BUTTON(self->_eButton))
    {
        self->_entity->setKeyValue(self->_key, "0");
    }
    else if (button == GTK_BUTTON(self->_seButton))
    {
        self->_entity->setKeyValue(self->_key, "315");
    }
    else if (button == GTK_BUTTON(self->_sButton))
    {
        self->_entity->setKeyValue(self->_key, "270");
    }
    else if (button == GTK_BUTTON(self->_swButton))
    {
        self->_entity->setKeyValue(self->_key, "225");
    }
    else if (button == GTK_BUTTON(self->_wButton))
    {
        self->_entity->setKeyValue(self->_key, "180");
    }
    else if (button == GTK_BUTTON(self->_nwButton))
    {
        self->_entity->setKeyValue(self->_key, "135");
    }
}



}

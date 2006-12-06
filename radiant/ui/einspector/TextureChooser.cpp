#include "TextureChooser.h"

#include "groupdialog.h"

#include <string>

namespace ui
{

// Construct the dialog

TextureChooser::TextureChooser(GtkWidget* entry, const std::string& prefixes)
: _entry(entry),
  _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	GtkWindow* gd = GroupDialog_getWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), gd);
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose texture");

	// Set the default size of the window
	
	gint w, h;
	gtk_window_get_size(gd, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
	
	// Construct main VBox, and pack in LightTextureSelector and buttons panel
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), _selector, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);

	// Show all widgets
	gtk_widget_show_all(_widget);
}

// Construct the buttons
GtkWidget* TextureChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);
	return hbx;
}

/* GTK CALLBACKS */

void TextureChooser::callbackCancel(GtkWidget* w, TextureChooser* self) {
	delete self;
}

void TextureChooser::callbackOK(GtkWidget* w, TextureChooser* self) {
	gtk_entry_set_text(GTK_ENTRY(self->_entry), 
					   self->_selector.getSelection().c_str());
	delete self;
}

} // namespace ui

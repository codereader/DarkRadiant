#include "BulgePatchDialog.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "iradiant.h"
#include "string/string.h"

namespace {
	const char* WINDOW_TITLE = "Bulge Patch";
	const char* LABEL_TITLE = "Bulge Patch";
	const char* LABEL_NOISE = "Noise:";
	
	const int NOISE = 16;
}

namespace ui {

BulgePatchDialog::BulgePatchDialog() :
	_dialog(NULL)
{
	GtkWindow* parent = GlobalRadiant().getMainWindow();

	// Create the new dialog window with OK and CANCEL button    
  	_dialog = gtk_dialog_new_with_buttons(WINDOW_TITLE, parent,
                                         GTK_DIALOG_DESTROY_WITH_PARENT, 
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK,
                                         NULL);
    
    // Set the dialog properties
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(_dialog)->vbox), 6);
    gtk_container_set_border_width(GTK_CONTAINER(_dialog), 6);
    gtk_dialog_set_has_separator(GTK_DIALOG(_dialog), FALSE);
    
    // Create the title label (bold font)
    GtkWidget* topLabel = gtk_label_new(NULL);
    std::string markup = std::string("<span weight=\"bold\">") + LABEL_TITLE + "</span>";
    gtk_label_set_markup(GTK_LABEL(topLabel), markup.c_str());
    gtk_misc_set_alignment(GTK_MISC(topLabel), 0.0f, 0.5f);
    gtk_misc_set_padding(GTK_MISC(topLabel), 6, 2);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(_dialog)->vbox), topLabel, TRUE, TRUE, 0);
    
    // Create the labels for the combo boxes
	GtkWidget* labelNoise = gtk_label_new(LABEL_NOISE);
	gtk_misc_set_alignment(GTK_MISC(labelNoise), 0.0f, 0.5f);
	
	// Create the text box
	_noiseEntry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(_noiseEntry), floatToStr(NOISE).c_str());
	
	// Create a new 2x5 table and pack it into an alignment
	GtkWidget* alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
	
	// Setup the table with default spacings
	GtkTable* table = GTK_TABLE(gtk_table_new(2, 5, FALSE));
    gtk_table_set_col_spacings(table, 12);
    gtk_table_set_row_spacings(table, 6);
    
    // Indent the table by adding a left-padding to the alignment
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 18, 6); 
    gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(table));
    
    // Pack the buttons into the table
	gtk_table_attach_defaults(table, labelNoise, 0, 1, 0, 1);
	gtk_table_attach_defaults(table, _noiseEntry, 1, 2, 0, 1);
	
	// Pack the table into the dialog
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(_dialog)->vbox), GTK_WIDGET(alignment), TRUE, TRUE, 0);
	
	// Make the OK button to the default response 
	gtk_dialog_set_default_response(GTK_DIALOG(_dialog), GTK_RESPONSE_OK);
	
	// Catch key events so that ENTER/ESC do as they are expected
	g_signal_connect(G_OBJECT(_dialog), "key-press-event", G_CALLBACK(onKeyPress), this);
}

bool BulgePatchDialog::queryPatchNoise(int& noise) {
	// Default return value is "cancelled"
	bool returnValue = false;

	// Instantiate a dialog and run the GTK dialog routine
	BulgePatchDialog dialog;

	gtk_widget_show_all(dialog._dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog._dialog));
	
	if (response == GTK_RESPONSE_OK) {
		// Retrieve the maxValue
		noise = strToFloat(gtk_entry_get_text(GTK_ENTRY(dialog._noiseEntry)));
		
		returnValue = true;
	}
	
	gtk_widget_destroy(GTK_WIDGET(dialog._dialog));
	
	return returnValue;
}

gboolean BulgePatchDialog::onKeyPress(GtkWidget* widget, GdkEventKey* event, BulgePatchDialog* self) {
	// Check for ESC and ENTER keys
	if (event->keyval == GDK_Escape) {
		gtk_dialog_response(GTK_DIALOG(self->_dialog), GTK_RESPONSE_REJECT);
	}
	else if (event->keyval == GDK_Return) {
		gtk_dialog_response(GTK_DIALOG(self->_dialog), GTK_RESPONSE_OK);
	}
	
	return false;
}

} // namespace ui

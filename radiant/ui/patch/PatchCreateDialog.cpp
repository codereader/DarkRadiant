#include "PatchCreateDialog.h"

#include "iradiant.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "string/string.h"

namespace {
	const char* WINDOW_TITLE = "Create Flat Patch Mesh";
	const char* LABEL_TITLE = "Create Simple Patch Mesh";
	const char* LABEL_WIDTH_COMBO = "Width: ";
	const char* LABEL_HEIGHT_COMBO = "Height: ";
	const char* LABEL_REMOVE_BRUSHES = "Remove selected Brush";
	
	const bool DEFAULT_REMOVE_BRUSHES = false;
	
	const int MIN_PATCH_DIM = 3;
	const int MAX_PATCH_DIM = 15;
	const int INCR_PATCH_DIM = 2;
}

namespace ui {

PatchCreateDialog::PatchCreateDialog() :
	_parent(GlobalRadiant().getMainWindow()),
	_dialog(NULL)
{
	// Create the new dialog window with OK and CANCEL button    
  	_dialog = gtk_dialog_new_with_buttons(WINDOW_TITLE, _parent,
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
	GtkWidget* labelWidth = gtk_label_new(LABEL_WIDTH_COMBO);
	GtkWidget* labelHeight = gtk_label_new(LABEL_HEIGHT_COMBO);
	gtk_misc_set_alignment(GTK_MISC(labelWidth), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(labelHeight), 0.0f, 0.5f);
	
	// Create the two combo boxes for width and height
	_comboWidth = gtk_combo_box_new_text();
	_comboHeight = gtk_combo_box_new_text();
	
	// Fill the values into the combo boxes
	for (int i = MIN_PATCH_DIM; i <= MAX_PATCH_DIM; i += INCR_PATCH_DIM) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(_comboWidth), intToStr(i).c_str());
		gtk_combo_box_append_text(GTK_COMBO_BOX(_comboHeight), intToStr(i).c_str());
	}
	
	// Activate the first item in the combo boxes
	gtk_combo_box_set_active(GTK_COMBO_BOX(_comboWidth), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(_comboHeight), 0);
	
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
	gtk_table_attach_defaults(table, labelWidth, 0, 1, 0, 1);
	gtk_table_attach_defaults(table, _comboWidth, 1, 2, 0, 1);
	gtk_table_attach_defaults(table, labelHeight, 0, 1, 1, 2);
	gtk_table_attach_defaults(table, _comboHeight, 1, 2, 1, 2);
	
	// Create the "create seams" label
	_removeBrushCheckbox = gtk_check_button_new_with_mnemonic(LABEL_REMOVE_BRUSHES);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_removeBrushCheckbox), DEFAULT_REMOVE_BRUSHES);
	gtk_table_attach_defaults(table, _removeBrushCheckbox, 0, 2, 2, 3);
	
	// Pack the table into the dialog
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(_dialog)->vbox), GTK_WIDGET(alignment), TRUE, TRUE, 0);
	
	// Make the OK button to the default response 
	gtk_dialog_set_default_response(GTK_DIALOG(_dialog), GTK_RESPONSE_OK);
	
	// Catch key events so that ENTER/ESC do as they are expected
	g_signal_connect(G_OBJECT(_dialog), "key-press-event", G_CALLBACK(onKeyPress), this);
}

bool PatchCreateDialog::queryPatchDimensions(int& width, int& height, 
							const int& selBrushCount, bool& removeBrush) 
{
	bool returnValue = false;
	
	// Activate/Inactivate the check box depending on the selected brush count
	if (selBrushCount == 1) {
		gtk_widget_set_sensitive(_removeBrushCheckbox, TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_removeBrushCheckbox), TRUE);
	}
	else {
		gtk_widget_set_sensitive(_removeBrushCheckbox, FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_removeBrushCheckbox), FALSE);
	}
	
	gtk_widget_show_all(_dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(_dialog));
		
	if (response == GTK_RESPONSE_OK) {
		// Retrieve the width/height from the widgets
		width = strToInt(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_comboWidth)));
		height = strToInt(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_comboHeight)));
		
		removeBrush = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_removeBrushCheckbox)) ? true : false;
		
		returnValue = true;
	}
	
	gtk_widget_destroy(GTK_WIDGET(_dialog));
	
	return returnValue;
}

gboolean PatchCreateDialog::onKeyPress(GtkWidget* widget, GdkEventKey* event, PatchCreateDialog* self) {
	
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

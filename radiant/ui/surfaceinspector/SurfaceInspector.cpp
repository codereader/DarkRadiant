#include "SurfaceInspector.h"

#include <gtk/gtk.h>
#include "gtkutil/TransientWindow.h"
#include "gtkutil/IconTextButton.h"
#include "mainframe.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Surface Inspector";
		const std::string LABEL_PROPERTIES = "Texture Properties";
		const std::string LABEL_OPERATIONS = "Texture Operations";
		
		const std::string HSHIFT = "horizshift";
		const std::string VSHIFT = "vertshift";
		const std::string HSCALE = "horziscale";
		const std::string VSCALE = "vertscale";
		const std::string ROTATION = "rotation";
	
		const std::string LABEL_HSHIFT = "Horiz. Shift:";
		const std::string LABEL_VSHIFT = "Vert. Shift:";
		const std::string LABEL_HSCALE = "Horiz. Scale:";
		const std::string LABEL_VSCALE = "Vert. Scale:";
		const std::string LABEL_ROTATION = "Rotation:";
		const char* LABEL_SHADER = "Shader:";
		const char* LABEL_STEP = "Step:";
		
		const char* LABEL_FIT_TEXTURE = "Fit Texture:";
		const char* LABEL_FIT = "Fit";
		
		const char* LABEL_FLIP_TEXTURE = "Flip Texture:";
		const char* LABEL_FLIPX = "Flip Horizontal";
		const char* LABEL_FLIPY = "Flip Vertical";
				
		const char* LABEL_PATCHES = "Patches:";
		const char* LABEL_NATURAL = "Natural";
		const char* LABEL_CAP = "Cycle CAP";
		
		const char* LABEL_BRUSHES = "Brushes:";
		const char* LABEL_AXIAL = "Axial";
		const char* LABEL_TEXTURE_LOCK = "Texture Lock";
	}

SurfaceInspector::SurfaceInspector() {
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	gtk_window_set_resizable(GTK_WINDOW(_dialog), true);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 6);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	populateWindow();
}

void SurfaceInspector::toggle() {
	gtk_widget_show_all(_dialog);
}

void SurfaceInspector::populateWindow() {
	// Create the overall vbox
	GtkWidget* dialogVBox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(_dialog), dialogVBox);
	
	// Create the title label (bold font)
    GtkWidget* topLabel = gtk_label_new(NULL);
    	std::string markup = std::string("<span weight=\"bold\">") + 
    						 LABEL_PROPERTIES + "</span>";
    	gtk_label_set_markup(GTK_LABEL(topLabel), markup.c_str());
    	gtk_misc_set_alignment(GTK_MISC(topLabel), 0.0f, 0.5f);
    	gtk_misc_set_padding(GTK_MISC(topLabel), 6, 2);
    
    gtk_box_pack_start(GTK_BOX(dialogVBox), topLabel, true, true, 0);
    
    // Create a new 2x6 table and pack it into an alignment
	GtkWidget* alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
	
		// Setup the table with default spacings
		GtkTable* table = GTK_TABLE(gtk_table_new(6, 2, false));
    	gtk_table_set_col_spacings(table, 12);
    	gtk_table_set_row_spacings(table, 6);
    
    	// Indent the table by adding a left-padding to the alignment
    	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 18, 6); 
    	gtk_container_add(GTK_CONTAINER(alignment), GTK_WIDGET(table));
    
    // Pack the table into the dialog
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(alignment), true, true, 0);
	
	// Create the entry field and pack it into the first table row
	GtkWidget* shaderLabel = gtk_label_new(LABEL_SHADER);
	gtk_misc_set_alignment(GTK_MISC(shaderLabel), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, shaderLabel, 0, 1, 0, 1);
	
	_shaderEntry = gtk_entry_new();
	//gtk_entry_set_width_chars(GTK_ENTRY(_shaderEntry), 40);
	gtk_table_attach_defaults(table, _shaderEntry, 1, 2, 0, 1);
	
	// Populate the table with the according widgets
	_manipulators[HSHIFT] = createManipulatorRow(LABEL_HSHIFT, table, 1);
	_manipulators[VSHIFT] = createManipulatorRow(LABEL_VSHIFT, table, 2);
	_manipulators[HSCALE] = createManipulatorRow(LABEL_HSCALE, table, 3);
	_manipulators[VSCALE] = createManipulatorRow(LABEL_VSCALE, table, 4);
	_manipulators[ROTATION] = createManipulatorRow(LABEL_ROTATION, table, 5);
	
	// ======================== Texture Operations ====================================
	
	// Create the texture operations label (bold font)
    GtkWidget* operLabel = gtk_label_new(NULL);
    	std::string operMarkup = std::string("<span weight=\"bold\">") + 
    						 LABEL_OPERATIONS + "</span>";
    	gtk_label_set_markup(GTK_LABEL(operLabel), operMarkup.c_str());
    	gtk_misc_set_alignment(GTK_MISC(operLabel), 0.0f, 0.5f);
    	gtk_misc_set_padding(GTK_MISC(operLabel), 6, 2);
    
    gtk_box_pack_start(GTK_BOX(dialogVBox), operLabel, true, true, 0);
    
    // Create a new 2x4 table and pack it into another alignment
	GtkWidget* operAlignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
	
		// Setup the table with default spacings
		GtkTable* operTable = GTK_TABLE(gtk_table_new(4, 2, false));
    	gtk_table_set_col_spacings(operTable, 12);
    	gtk_table_set_row_spacings(operTable, 6);
    
    	// Indent the table by adding a left-padding to the alignment
    	gtk_alignment_set_padding(GTK_ALIGNMENT(operAlignment), 0, 0, 18, 6); 
    	gtk_container_add(GTK_CONTAINER(operAlignment), GTK_WIDGET(operTable));
    
    // Pack the table into the dialog
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(operAlignment), true, true, 0);
	
	// ------------------------ Fit Texture -----------------------------------
	
	GtkWidget* fitHBox = gtk_hbox_new(false, 6); 
	
	// Create the "Fit Texture" label
	_fitTexture.label = gtk_label_new(LABEL_FIT_TEXTURE);
	gtk_misc_set_alignment(GTK_MISC(_fitTexture.label), 0.0f, 0.5f);
	gtk_table_attach_defaults(operTable, _fitTexture.label, 0, 1, 0, 1);
	
	_fitTexture.widthAdj = gtk_adjustment_new(1.0f, 0.0f, 1000.0f, 1.0f, 1.0f, 1.0f);
	_fitTexture.heightAdj = gtk_adjustment_new(1.0f, 0.0f, 1000.0f, 1.0f, 1.0f, 1.0f);
	
	// Create the width entry field
	_fitTexture.width = gtk_spin_button_new(GTK_ADJUSTMENT(_fitTexture.widthAdj), 1.0f, 4);
	gtk_widget_set_size_request(_fitTexture.width, 55, -1);
	gtk_box_pack_start(GTK_BOX(fitHBox), _fitTexture.width, false, false, 0);
	
	// Create the "x" label
	GtkWidget* xLabel = gtk_label_new("x");
	gtk_misc_set_alignment(GTK_MISC(xLabel), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(fitHBox), xLabel, false, false, 0);
	
	// Create the height entry field
	_fitTexture.height = gtk_spin_button_new(GTK_ADJUSTMENT(_fitTexture.heightAdj), 1.0f, 4);
	gtk_widget_set_size_request(_fitTexture.height, 55, -1);
	gtk_box_pack_start(GTK_BOX(fitHBox), _fitTexture.height, false, false, 0);
	
	_fitTexture.button = gtk_button_new_with_label(LABEL_FIT);
	gtk_widget_set_size_request(_fitTexture.button, 30, -1);
	gtk_box_pack_start(GTK_BOX(fitHBox), _fitTexture.button, true, true, 0);
		
	gtk_table_attach_defaults(operTable, fitHBox, 1, 2, 0, 1);
	
	// ------------------------ Operation Buttons ------------------------------
	
	// Create the "Flip Texture" label
	_flipTexture.label = gtk_label_new(LABEL_FLIP_TEXTURE);
	gtk_misc_set_alignment(GTK_MISC(_flipTexture.label), 0.0f, 0.5f);
	gtk_table_attach_defaults(operTable, _flipTexture.label, 0, 1, 1, 2);
	
	_flipTexture.hbox = gtk_hbox_new(true, 6); 
	_flipTexture.flipX = gtk_button_new_with_label(LABEL_FLIPX);
	_flipTexture.flipY = gtk_button_new_with_label(LABEL_FLIPY);
	gtk_box_pack_start(GTK_BOX(_flipTexture.hbox), _flipTexture.flipX, true, true, 0);
	gtk_box_pack_start(GTK_BOX(_flipTexture.hbox), _flipTexture.flipY, true, true, 0);
	
	gtk_table_attach_defaults(operTable, _flipTexture.hbox, 1, 2, 1, 2);
	
	// Create the "Patches" label
	_patches.label = gtk_label_new(LABEL_PATCHES);
	gtk_misc_set_alignment(GTK_MISC(_patches.label), 0.0f, 0.5f);
	gtk_table_attach_defaults(operTable, _patches.label, 0, 1, 2, 3);
	
	_patches.hbox = gtk_hbox_new(true, 6); 
	_patches.natural = gtk_button_new_with_label(LABEL_NATURAL);
	_patches.cycleCap = gtk_button_new_with_label(LABEL_CAP);
	gtk_box_pack_start(GTK_BOX(_patches.hbox), _patches.natural, true, true, 0);
	gtk_box_pack_start(GTK_BOX(_patches.hbox), _patches.cycleCap, true, true, 0);
	
	gtk_table_attach_defaults(operTable, _patches.hbox, 1, 2, 2, 3);
	
	// Brushes
	_brushes.label = gtk_label_new(LABEL_BRUSHES);
	gtk_misc_set_alignment(GTK_MISC(_brushes.label), 0.0f, 0.5f);
	gtk_table_attach_defaults(operTable, _brushes.label, 0, 1, 3, 4);
	
	_brushes.hbox = gtk_hbox_new(true, 6); 
	_brushes.axial = gtk_button_new_with_label(LABEL_AXIAL);
	_brushes.texLock = gtk_toggle_button_new_with_label(LABEL_TEXTURE_LOCK);
	gtk_box_pack_start(GTK_BOX(_brushes.hbox), _brushes.axial, true, true, 0);
	gtk_box_pack_start(GTK_BOX(_brushes.hbox), _brushes.texLock, true, true, 0);
	
	gtk_table_attach_defaults(operTable, _brushes.hbox, 1, 2, 3, 4);
}

SurfaceInspector::ManipulatorRow SurfaceInspector::createManipulatorRow(
	const std::string& label, GtkTable* table, int row) 
{
	ManipulatorRow manipRow;
	
	manipRow.hbox = gtk_hbox_new(false, 6);
		
	// Create the label
	manipRow.label = gtk_label_new(label.c_str());
	gtk_misc_set_alignment(GTK_MISC(manipRow.label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, manipRow.label, 0, 1, row, row+1);
		
	// Create the entry field
	manipRow.value = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(manipRow.value), 7);
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.value, true, true, 0);
	
	manipRow.leftArrow = gtkutil::IconTextButton("", "left_arrow.png", false);
	gtk_widget_set_size_request(manipRow.leftArrow, -1, -1);
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.leftArrow, false, false, 0);
	
	manipRow.rightArrow = gtkutil::IconTextButton("", "right_arrow.png", false);
	gtk_widget_set_size_request(manipRow.rightArrow, -1, -1);
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.rightArrow, false, false, 0);
	
	// Create the label
	manipRow.steplabel = gtk_label_new(LABEL_STEP);
	gtk_misc_set_alignment(GTK_MISC(manipRow.steplabel), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.steplabel, false, false, 0);
	
	// Create the entry field
	manipRow.step = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(manipRow.step), 5);
	gtk_box_pack_start(GTK_BOX(manipRow.hbox), manipRow.step, false, false, 0);
	
	// Packt the hbox into the table
	gtk_table_attach_defaults(table, manipRow.hbox, 1, 2, row, row+1);
	
	// Return the filled structure
	return manipRow;
}

void SurfaceInspector::toggleInspector() {
	// The static instance
	static SurfaceInspector _inspector;

	// Now toggle the dialog
	//_inspector.toggle();
}

gboolean SurfaceInspector::onDelete(GtkWidget* widget, GdkEvent* event, SurfaceInspector* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

} // namespace ui

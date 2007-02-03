#include "OverlayDialog.h"
#include "mainframe.h"

#include "ioverlay.h"
#include "iscenegraph.h"
#include "iregistry.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */
namespace {
	
	const char* DIALOG_TITLE = "Background image";

	// Registry keys
	const char* RKEY_OVERLAY_VISIBLE = "user/ui/xyview/overlay/visible";
	
	const char* 
	RKEY_OVERLAY_TRANSPARENCY = "user/ui/xyview/overlay/transparency";
	
	const char* RKEY_OVERLAY_IMAGE = "user/ui/xyview/overlay/image";
	const char* RKEY_OVERLAY_SCALE = "user/ui/xyview/overlay/scale";
	
	const char* 
	RKEY_OVERLAY_TRANSLATIONX = "user/ui/xyview/overlay/translationX";
	
	const char* 
	RKEY_OVERLAY_TRANSLATIONY = "user/ui/xyview/overlay/translationY";
	
	const char* 
	RKEY_OVERLAY_PROPORTIONAL = "user/ui/xyview/overlay/proportional";
	
	const char* 
	RKEY_OVERLAY_SCALE_WITH_XY = "user/ui/xyview/overlay/scaleWithOrthoView";

}

// Create GTK stuff in c-tor
OverlayDialog::OverlayDialog()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Set up the window
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
    gtk_window_set_title(GTK_WINDOW(_widget), DIALOG_TITLE);
    g_signal_connect(G_OBJECT(_widget), "delete-event",
    				 G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	// Set default size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 3, -1);

	// Pack in widgets
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), createWidgets(), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);	
}

// Construct main widgets
GtkWidget* OverlayDialog::createWidgets() {
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	
	// "Use image" checkbox
	GtkWidget* useImage = gtk_check_button_new_with_label(
							"Use background image"); 
	_subWidgets["useImage"] = useImage;
	g_signal_connect(G_OBJECT(useImage), "toggled",
					 G_CALLBACK(_onUseImage), this);
	
	gtk_box_pack_start(GTK_BOX(vbx), useImage, FALSE, FALSE, 0);
	
	// Other widgets are in a table, which is indented with respect to the
	// Use Image checkbox, and becomes enabled/disabled with it.
	GtkWidget* tbl = gtk_table_new(5, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(tbl), 12);
	gtk_table_set_col_spacings(GTK_TABLE(tbl), 12);
	_subWidgets["subTable"] = tbl;
	
	// Image file
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Image file</b>"),
					 0, 1, 0, 1, 
					 GTK_FILL, GTK_FILL, 0, 0);
	
	GtkWidget* fileButton = gtk_file_chooser_button_new(
							  	"Choose image", GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(fileButton), "selection-changed",
					 G_CALLBACK(_onFileSelection), this);
	_subWidgets["fileChooser"] = fileButton;
	
	gtk_table_attach_defaults(GTK_TABLE(tbl), fileButton, 1, 2, 0, 1);
	
	// Transparency slider
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Transparency</b>"),
					 0, 1, 1, 2, 
					 GTK_FILL, GTK_FILL, 0, 0);
				
	GtkWidget* transSlider = gtk_hscale_new_with_range(0, 1, 0.1);
	g_signal_connect(G_OBJECT(transSlider), "value-changed",
					 G_CALLBACK(_onTransparencyScroll), this);
	_subWidgets["transparency"] = transSlider;
							  				
	gtk_table_attach_defaults(GTK_TABLE(tbl), transSlider, 1, 2, 1, 2);
	
	// Image size slider
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Image scale</b>"),
					 0, 1, 2, 3, 
					 GTK_FILL, GTK_FILL, 0, 0);

	GtkWidget* scale = gtk_hscale_new_with_range(0, 20, 0.1);
	g_signal_connect(G_OBJECT(scale), "value-changed",
					 G_CALLBACK(_onScaleScroll), this);
	_subWidgets["scale"] = scale;
							  				
	gtk_table_attach_defaults(GTK_TABLE(tbl), scale, 1, 2, 2, 3);
	
	// Options list
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Options</b>"),
					 0, 1, 3, 4, 
					 GTK_FILL, GTK_FILL, 0, 0);
	
	GtkWidget* keepAspect = 
		gtk_check_button_new_with_label("Keep aspect ratio"); 
	g_signal_connect(G_OBJECT(keepAspect), "toggled",
					 G_CALLBACK(_onKeepAspect), this);
	_subWidgets["keepAspect"] = keepAspect;
	gtk_table_attach_defaults(GTK_TABLE(tbl), keepAspect, 1, 2, 3, 4);
	
	GtkWidget* scaleWithViewport =
		gtk_check_button_new_with_label("Zoom image with viewport");
	g_signal_connect(G_OBJECT(scaleWithViewport), "toggled",
					 G_CALLBACK(_onScaleImage), this);
	_subWidgets["scaleImage"] = scaleWithViewport;	
	gtk_table_attach_defaults(GTK_TABLE(tbl), scaleWithViewport, 1, 2, 4, 5);
	
	// Pack table into vbox and return
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtkutil::LeftAlignment(tbl, 18, 1.0), 
					   TRUE, TRUE, 0);				
	
	return vbx;
}

// Create button panel
GtkWidget* OverlayDialog::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	
	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(closeButton), "clicked",
					 G_CALLBACK(_onClose), this); 
	
	gtk_box_pack_end(GTK_BOX(hbx), closeButton, FALSE, FALSE, 0);
	return hbx;
}

// Static show method
void OverlayDialog::display() {
	
	// Maintain a static dialog instance and display it on demand
	static OverlayDialog _instance;
	
	// Update the dialog state from the registry, and show it
	_instance.getStateFromRegistry();
	gtk_widget_show_all(_instance._widget);
}

// Get the dialog state from the registry
void OverlayDialog::getStateFromRegistry() {
	
	// Use image
	if (GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1") {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_subWidgets["useImage"]),
									 TRUE);
		gtk_widget_set_sensitive(_subWidgets["subTable"], TRUE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_subWidgets["useImage"]),
									 FALSE);
		gtk_widget_set_sensitive(_subWidgets["subTable"], FALSE);
	}
	
	// Image filename
	gtk_file_chooser_set_filename(
			GTK_FILE_CHOOSER(_subWidgets["fileChooser"]),
			GlobalRegistry().get(RKEY_OVERLAY_IMAGE).c_str());
			
	// Transparency
	gtk_range_set_value(
		GTK_RANGE(_subWidgets["transparency"]),
		boost::lexical_cast<double>(
			GlobalRegistry().get(RKEY_OVERLAY_TRANSPARENCY)));
	
	// Image scale
	gtk_range_set_value(
		GTK_RANGE(_subWidgets["scale"]),
		boost::lexical_cast<double>(GlobalRegistry().get(RKEY_OVERLAY_SCALE)));
			
	// Options: Keep aspect
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_subWidgets["keepAspect"]),
		GlobalRegistry().get(RKEY_OVERLAY_PROPORTIONAL) == "1" ? TRUE : FALSE);
	
	// Options: Scale with window
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_subWidgets["scaleImage"]),
		GlobalRegistry().get(RKEY_OVERLAY_SCALE_WITH_XY) == "1" ? TRUE : FALSE);
}

/* GTK CALLBACKS */

// Close button
void OverlayDialog::_onClose(GtkWidget* w, OverlayDialog* self) {
	gtk_widget_hide(self->_widget);
}

// Keep aspect toggle
void OverlayDialog::_onKeepAspect(GtkToggleButton* w, OverlayDialog* self) {

	// Set the registry key
	if (gtk_toggle_button_get_active(w)) {
		GlobalRegistry().set(RKEY_OVERLAY_PROPORTIONAL, "1");
	}
	else {
		GlobalRegistry().set(RKEY_OVERLAY_PROPORTIONAL, "0");
	}	

	// Refresh
	GlobalSceneGraph().sceneChanged();
}

// Scale with viewport toggle
void OverlayDialog::_onScaleImage(GtkToggleButton* w, OverlayDialog* self) {

	// Set the registry key
	if (gtk_toggle_button_get_active(w)) {
		GlobalRegistry().set(RKEY_OVERLAY_SCALE_WITH_XY, "1");
	}
	else {
		GlobalRegistry().set(RKEY_OVERLAY_SCALE_WITH_XY, "0");
	}	

}

// Use image toggle
void OverlayDialog::_onUseImage(GtkToggleButton* w, OverlayDialog* self) {
	
	// Enable or disable the overlay based on checked status, also update the
	// sensitivity of the subtable
	if (gtk_toggle_button_get_active(w)) {
		GlobalRegistry().set(RKEY_OVERLAY_VISIBLE, "1");
		gtk_widget_set_sensitive(self->_subWidgets["subTable"], TRUE);
	}
	else {
		GlobalRegistry().set(RKEY_OVERLAY_VISIBLE, "0");
		gtk_widget_set_sensitive(self->_subWidgets["subTable"], FALSE);
	}	
	
	// Refresh
	GlobalSceneGraph().sceneChanged();
}

// File selection
void OverlayDialog::_onFileSelection(GtkFileChooser* w, OverlayDialog* self) {
	
	// Get the raw filename from GTK
	char* szFilename = gtk_file_chooser_get_filename(w);
	if (szFilename == NULL)
		return;
		
	// Convert to string and free the pointer
	std::string fileName(szFilename);
	g_free(szFilename);
	
	// Set registry key
	GlobalRegistry().set(RKEY_OVERLAY_IMAGE, fileName);	
}

// Transparency change
bool OverlayDialog::_onTransparencyScroll(GtkRange* w, 
										  GtkScrollType t,
										  double value,
										  OverlayDialog* self)
{
	// Get the value and set the transparency key
	std::string sVal = boost::lexical_cast<std::string>(gtk_range_get_value(w));
	GlobalRegistry().set(RKEY_OVERLAY_TRANSPARENCY, sVal);

	// Refresh display
	GlobalSceneGraph().sceneChanged();

	// Don't stop signal handling for this event
	return FALSE;
}

// Image scale change
bool OverlayDialog::_onScaleScroll(GtkRange* w, 
								   GtkScrollType t,
								   double value,
								   OverlayDialog* self)
{
	// Get the value and set the size key
	std::string sVal = boost::lexical_cast<std::string>(gtk_range_get_value(w));
	GlobalRegistry().set(RKEY_OVERLAY_SCALE, sVal);

	// Refresh display
	GlobalSceneGraph().sceneChanged();

	// Don't stop signal handling for this event
	return FALSE;
}


} // namespace ui

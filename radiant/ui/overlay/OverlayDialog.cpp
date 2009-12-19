#include "OverlayDialog.h"

#include "imainframe.h"
#include "iscenegraph.h"
#include "iregistry.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/SerialisableWidgets.h"

#include <gtk/gtk.h>

#include "OverlayRegistryKeys.h"

namespace ui
{

/* CONSTANTS */
namespace {
	const char* DIALOG_TITLE = "Background image";
}

// Create GTK stuff in c-tor
OverlayDialog::OverlayDialog() : 
	_widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
	_callbackActive(false)
{
	// Set up the window
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_transient_for(GTK_WINDOW(_widget), GlobalMainFrame().getTopLevelWindow());
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
	
	// Connect the widgets to the registry
	connectWidgets();	
}

// Construct main widgets
GtkWidget* OverlayDialog::createWidgets() {
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	
	// "Use image" checkbox
	GtkWidget* useImage = gtk_check_button_new_with_label(
							"Use background image"); 
	_subWidgets["useImage"] = useImage;
	g_signal_connect(G_OBJECT(useImage), "toggled",
					 G_CALLBACK(_onChange), this);
	
	gtk_box_pack_start(GTK_BOX(vbx), useImage, FALSE, FALSE, 0);
	
	// Other widgets are in a table, which is indented with respect to the
	// Use Image checkbox, and becomes enabled/disabled with it.
	GtkWidget* tbl = gtk_table_new(8, 2, FALSE);
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
				
	GtkWidget* transSlider = gtk_hscale_new_with_range(0, 1, 0.01);
	g_signal_connect(G_OBJECT(transSlider), "value-changed",
					 G_CALLBACK(_onScrollChange), this);
	_subWidgets["transparency"] = transSlider;
							  				
	gtk_table_attach_defaults(GTK_TABLE(tbl), transSlider, 1, 2, 1, 2);
	
	// Image size slider
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Image scale</b>"),
					 0, 1, 2, 3, 
					 GTK_FILL, GTK_FILL, 0, 0);

	GtkWidget* scale = gtk_hscale_new_with_range(0, 20, 0.01);
	g_signal_connect(G_OBJECT(scale), "value-changed",
					 G_CALLBACK(_onScrollChange), this);
	_subWidgets["scale"] = scale;
							  				
	gtk_table_attach_defaults(GTK_TABLE(tbl), scale, 1, 2, 2, 3);
	
	// Translation X slider
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Horizontal offset</b>"),
					 0, 1, 3, 4, 
					 GTK_FILL, GTK_FILL, 0, 0);

	GtkWidget* transx = gtk_hscale_new_with_range(-20, 20, 0.01);
	g_signal_connect(G_OBJECT(transx), "value-changed",
					 G_CALLBACK(_onScrollChange), this);
	_subWidgets["translateX"] = transx;
							  				
	gtk_table_attach_defaults(GTK_TABLE(tbl), transx, 1, 2, 3, 4);
	
	// Translation Y slider
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Vertical offset</b>"),
					 0, 1, 4, 5, 
					 GTK_FILL, GTK_FILL, 0, 0);

	GtkWidget* transy = gtk_hscale_new_with_range(-20, 20, 0.01);
	g_signal_connect(G_OBJECT(transy), "value-changed",
					 G_CALLBACK(_onScrollChange), this);
	_subWidgets["translateY"] = transy;
	
	gtk_table_attach_defaults(GTK_TABLE(tbl), transy, 1, 2, 4, 5);
	
	// Options list
	gtk_table_attach(GTK_TABLE(tbl), 
					 gtkutil::LeftAlignedLabel("<b>Options</b>"),
					 0, 1, 5, 6, 
					 GTK_FILL, GTK_FILL, 0, 0);
	
	GtkWidget* keepAspect = 
		gtk_check_button_new_with_label("Keep aspect ratio"); 
	g_signal_connect(G_OBJECT(keepAspect), "toggled",
					 G_CALLBACK(_onChange), this);
	_subWidgets["keepAspect"] = keepAspect;
	gtk_table_attach_defaults(GTK_TABLE(tbl), keepAspect, 1, 2, 5, 6);
	
	GtkWidget* scaleWithViewport =
		gtk_check_button_new_with_label("Zoom image with viewport");
	g_signal_connect(G_OBJECT(scaleWithViewport), "toggled",
					 G_CALLBACK(_onChange), this);
	_subWidgets["scaleImage"] = scaleWithViewport;	
	gtk_table_attach_defaults(GTK_TABLE(tbl), scaleWithViewport, 1, 2, 6, 7);
	
	GtkWidget* panWithViewport =
		gtk_check_button_new_with_label("Pan image with viewport");
	g_signal_connect(G_OBJECT(panWithViewport), "toggled",
					 G_CALLBACK(_onChange), this);
	_subWidgets["panImage"] = panWithViewport;	
	gtk_table_attach_defaults(GTK_TABLE(tbl), panWithViewport, 1, 2, 7, 8);
	
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
void OverlayDialog::display(const cmd::ArgumentList& args) {
	
	// Maintain a static dialog instance and display it on demand
	static OverlayDialog _instance;
	
	// Update the dialog state from the registry, and show it
	_instance.getStateFromRegistry();
	gtk_widget_show_all(_instance._widget);
}

void OverlayDialog::connectWidgets() {
   using namespace gtkutil;
	_connector.addObject(
        RKEY_OVERLAY_VISIBLE,
        SerialisableWidgetWrapperPtr(
            new SerialisableToggleButton(_subWidgets["useImage"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_TRANSPARENCY,
        SerialisableWidgetWrapperPtr(
            new SerialisableScaleWidget(_subWidgets["transparency"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_SCALE,
        SerialisableWidgetWrapperPtr(
            new SerialisableScaleWidget(_subWidgets["scale"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_PROPORTIONAL,
        SerialisableWidgetWrapperPtr(
            new SerialisableToggleButton(_subWidgets["keepAspect"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_SCALE_WITH_XY,
        SerialisableWidgetWrapperPtr(
            new SerialisableToggleButton(_subWidgets["scaleImage"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_PAN_WITH_XY,
        SerialisableWidgetWrapperPtr(
            new SerialisableToggleButton(_subWidgets["panImage"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_TRANSLATIONX,
        SerialisableWidgetWrapperPtr(
            new SerialisableScaleWidget(_subWidgets["translateX"])
        )
    );
	_connector.addObject(
        RKEY_OVERLAY_TRANSLATIONY,
        SerialisableWidgetWrapperPtr(
            new SerialisableScaleWidget(_subWidgets["translateY"])
        )
    );
}

// Get the dialog state from the registry
void OverlayDialog::getStateFromRegistry() {
	
	// Load the values into the widgets
	_callbackActive = true;
	_connector.importValues();
	updateSensitivity();
		
	// Image filename
	gtk_file_chooser_set_filename(
			GTK_FILE_CHOOSER(_subWidgets["fileChooser"]),
			GlobalRegistry().get(RKEY_OVERLAY_IMAGE).c_str());
	
	_callbackActive = false;
}

void OverlayDialog::updateSensitivity() {
	// If the "Use image" toggle is disabled, desensitise all the other widgets
	gtk_widget_set_sensitive(
		_subWidgets["subTable"],
		GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1"
	);
}

/* GTK CALLBACKS */

// Close button
void OverlayDialog::_onClose(GtkWidget* w, OverlayDialog* self) {
	gtk_widget_hide(self->_widget);
}

// Generalised callback that triggers a value export
void OverlayDialog::_onChange(GtkWidget* w, OverlayDialog* self) {
	if (self->_callbackActive) {
		return;
	}
	// Export all the widget values to the registry
	self->_connector.exportValues();
	self->updateSensitivity();
	// Refresh
	GlobalSceneGraph().sceneChanged();
}

// File selection
void OverlayDialog::_onFileSelection(GtkFileChooser* w, OverlayDialog* self) {
	if (self->_callbackActive) {
		return;
	}
	
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

// Scroll changes (triggers an update)
void OverlayDialog::_onScrollChange(GtkWidget* range, OverlayDialog* self) {
	if (self->_callbackActive) {
		return;
	}
	
	// Export all the widget values to the registry
	self->_connector.exportValues();
	// Refresh display
	GlobalSceneGraph().sceneChanged();
}

} // namespace ui

#include "MapInfoDialog.h"

#include <gtk/gtk.h>
#include "ieventmanager.h"
#include "iradiant.h"

namespace ui {

	namespace {
		const int MAPINFO_DEFAULT_SIZE_X = 600;
	    const int MAPINFO_DEFAULT_SIZE_Y = 550;
	   	const std::string MAPINFO_WINDOW_TITLE = "Map Info";
	}

MapInfoDialog::MapInfoDialog() :
	BlockingTransientWindow(MAPINFO_WINDOW_TITLE, GlobalRadiant().getMainWindow())
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), MAPINFO_DEFAULT_SIZE_X, MAPINFO_DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets
	populateWindow();
	
	// Propagate shortcuts to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Show the window and its children, enter the main loop
	show();
}

void MapInfoDialog::shutdown() {
	// Stop propagating shortcuts to the main window
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));
}

void MapInfoDialog::populateWindow() {
	// Create the vbox containing the notebook and the buttons
	GtkWidget* dialogVBox = gtk_vbox_new(FALSE, 6);

	// Create the tabs
	_notebook = GTK_NOTEBOOK(gtk_notebook_new());

	// Entity Info
	gtk_notebook_append_page(
		_notebook, 
		_entityInfo.getWidget(), 
		createTabLabel(_entityInfo.getLabel(), _entityInfo.getIconName())
	);

	// Shader Info
	gtk_notebook_append_page(
		_notebook, 
		_shaderInfo.getWidget(), 
		createTabLabel(_shaderInfo.getLabel(), _shaderInfo.getIconName())
	);

	// Model Info
	gtk_notebook_append_page(
		_notebook, 
		_modelInfo.getWidget(), 
		createTabLabel(_modelInfo.getLabel(), _modelInfo.getIconName())
	);
	
	// Add notebook plus buttons to vbox
	gtk_box_pack_start(GTK_BOX(dialogVBox), GTK_WIDGET(_notebook), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(dialogVBox), createButtons(), FALSE, FALSE, 0);

	// Add vbox to dialog window
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
}

GtkWidget* MapInfoDialog::createTabLabel(const std::string& label, const std::string& iconName) {
	// The tab label items (icon + label)
	GtkWidget* hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(
    	GTK_BOX(hbox), 
    	gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(iconName)), 
    	FALSE, FALSE, 3
    );
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(label.c_str()), FALSE, FALSE, 3);

	// Show the widgets before using them as label, they won't appear otherwise
	gtk_widget_show_all(hbox);

	return hbox;
}

GtkWidget* MapInfoDialog::createButtons() {
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(onClose), this);

	gtk_box_pack_end(GTK_BOX(hbox), closeButton, FALSE, FALSE, 0);
	
	return hbox;
}

void MapInfoDialog::onClose(GtkWidget* widget, MapInfoDialog* self) {
	// Call the destroy method which exits the main loop
	self->shutdown();
	self->destroy();
}

void MapInfoDialog::showDialog() {
	MapInfoDialog dialog; // blocks on instantiation
}

} // namespace ui

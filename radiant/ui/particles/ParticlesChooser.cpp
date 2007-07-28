#include "ParticlesChooser.h"
#include "ParticlesVisitor.h"

#include "iparticles.h"

#include "mainframe.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"

#include <gtk/gtk.h>

namespace ui
{

// Create GTK widgets
ParticlesChooser::ParticlesChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _particlesList(gtk_list_store_new(1, G_TYPE_STRING)),
  _selectedParticle("")
{
	// Set up main window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose particles");
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	g_signal_connect(G_OBJECT(_widget), "delete-event", 
					 G_CALLBACK(_onDestroy), this);
	
	// Set the default size of the window
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 3, h / 2);
	
	// Main dialog vbox
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	
	// Add main vbox to dialog
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);	
}

// Create the tree view
GtkWidget* ParticlesChooser::createTreeView() {
	
	GtkWidget* tv = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_particlesList));
	
	// Single text column
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv),
								gtkutil::TextColumn("Particle", 0));
	
	// Populate with particle names
	populateParticleList();
	
	// Pack into scrolled window and return
	return gtkutil::ScrolledFrame(tv);
	
}

// Populate the particles list
void ParticlesChooser::populateParticleList() {
	
	// Create and use a ParticlesVisitor to populate the list
	ParticlesVisitor visitor(_particlesList);
	GlobalParticlesManager().forEachParticleDef(visitor);
}

// Static instance owner
ParticlesChooser& ParticlesChooser::getInstance() {
	static ParticlesChooser _instance;
	return _instance;
}

// Enter recursive main loop
void ParticlesChooser::showAndBlock() {
	gtk_widget_show_all(_widget);
	gtk_main();
}

// Choose a particle system
std::string ParticlesChooser::chooseParticle() {
	ParticlesChooser instance = getInstance();
	instance.showAndBlock();
	return instance._selectedParticle;
}

/* GTK CALLBACKS */
gboolean ParticlesChooser::_onDestroy(GtkWidget* w, 
									  GdkEvent* ev,
									  ParticlesChooser* self) 
{
	self->_selectedParticle = "";
	gtk_main_quit();
	gtk_widget_hide(self->_widget);
	return TRUE;
}


} // namespace ui

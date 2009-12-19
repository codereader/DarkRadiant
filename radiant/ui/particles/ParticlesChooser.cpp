#include "ParticlesChooser.h"
#include "ParticlesVisitor.h"

#include "imainframe.h"
#include "iparticles.h"

#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"

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
	gtk_window_set_transient_for(GTK_WINDOW(_widget), GlobalMainFrame().getTopLevelWindow());
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
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
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
	
	// Connect up the selection changed callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(
		G_OBJECT(_selection), "changed", G_CALLBACK(_onSelChanged), this
	);
	
	// Pack into scrolled window and return
	return gtkutil::ScrolledFrame(tv);
	
}

// Create the buttons panel
GtkWidget* ParticlesChooser::createButtons() {

	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", 
					 G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);	
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
					   
	return gtkutil::RightAlignment(hbx);	
}

// Populate the particles list
void ParticlesChooser::populateParticleList() {
	
	// Create and use a ParticlesVisitor to populate the list
	ParticlesVisitor visitor(_particlesList, _iterMap);
	GlobalParticlesManager().forEachParticleDef(visitor);
}

// Static instance owner
ParticlesChooser& ParticlesChooser::getInstance() {
	static ParticlesChooser _instance;
	return _instance;
}

// Enter recursive main loop
void ParticlesChooser::showAndBlock(const std::string& current) {
	
    _selectedParticle = "";

    // Highlight the current particle
    IterMap::const_iterator i = _iterMap.find(current);
    if (i != _iterMap.end()) {
        _selectedParticle = current;
        GtkTreeIter* selectionIter = i->second;
        gtk_tree_selection_select_iter(_selection, selectionIter);
    }
    
    // Show the widget
	gtk_widget_show_all(_widget);
	gtk_main();
}

// Choose a particle system
std::string ParticlesChooser::chooseParticle(const std::string& current) {
	ParticlesChooser& instance = getInstance();
	instance.showAndBlock(current);
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

void ParticlesChooser::_onOK(GtkWidget* w, ParticlesChooser* self) {
	gtk_widget_hide(self->_widget);
	gtk_main_quit();
}

void ParticlesChooser::_onCancel(GtkWidget* w, ParticlesChooser* self) { 
	
	// Clear the selection before returning
	self->_selectedParticle = "";
	
	gtk_widget_hide(self->_widget);
	gtk_main_quit();
}

void ParticlesChooser::_onSelChanged(GtkWidget* w, ParticlesChooser* self) {

	// Get the selection and store it
	self->_selectedParticle = 
		gtkutil::TreeModel::getSelectedString(self->_selection, 0);
}

} // namespace ui

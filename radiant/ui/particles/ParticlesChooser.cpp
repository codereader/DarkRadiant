#include "ParticlesChooser.h"

#include <gtk/gtk.h>

namespace ui
{

// Create GTK widgets
ParticlesChooser::ParticlesChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _selectedParticle("")
{
	g_signal_connect(G_OBJECT(_widget), "delete-event", 
					 G_CALLBACK(_onDestroy), this);
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
gboolean ParticlesChooser::_onDestroy(GtkWidget* w, ParticlesChooser* self) {
	self->_selectedParticle = "";
	gtk_main_quit();
	gtk_widget_hide(self->_widget);
	return TRUE;
}


} // namespace ui

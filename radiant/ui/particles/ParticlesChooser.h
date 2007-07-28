#ifndef PARTICLESCHOOSER_H_
#define PARTICLESCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>

#include <string>

namespace ui
{

/**
 * Chooser dialog for selection (and possibly preview) of particle systems.
 */
class ParticlesChooser
{
	// Main dialog
	GtkWidget* _widget;

	// Liststore for the main particles list
	GtkListStore* _particlesList;
	
	// Last selected particle
	std::string _selectedParticle;
	
private:
	
	/* GTK CALLBACKS */
	static gboolean _onDestroy(GtkWidget*, GdkEvent*, ParticlesChooser*);
	
	// Constructor creates GTK elements
	ParticlesChooser();
	
	/* WIDGET CONSTRUCTION */
	GtkWidget* createTreeView();
	
	// Static instance owner
	static ParticlesChooser& getInstance();
	
	// Show the widgets and enter recursive main loop
	void showAndBlock();

public:
	
	/**
	 * Display the singleton dialog and return the name of the selected 
	 * particle system, or the empty string if none was selected.
	 */
	static std::string chooseParticle();
	
};

}

#endif /*PARTICLESCHOOSER_H_*/

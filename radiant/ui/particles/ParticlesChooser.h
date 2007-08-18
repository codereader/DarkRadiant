#ifndef PARTICLESCHOOSER_H_
#define PARTICLESCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeselection.h>

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

	// Liststore for the main particles list, and its selection object
	GtkListStore* _particlesList;
	GtkTreeSelection* _selection;
	
	// Last selected particle
	std::string _selectedParticle;
	
private:
	
	/* GTK CALLBACKS */
	static gboolean _onDestroy(GtkWidget*, GdkEvent*, ParticlesChooser*);
	static void _onOK(GtkWidget*, ParticlesChooser*);
	static void _onCancel(GtkWidget*, ParticlesChooser*);
	static void _onSelChanged(GtkWidget*, ParticlesChooser*);
	
	// Constructor creates GTK elements
	ParticlesChooser();
	
	/* WIDGET CONSTRUCTION */
	GtkWidget* createTreeView();
	GtkWidget* createButtons();
	
	// Static instance owner
	static ParticlesChooser& getInstance();
	
	// Show the widgets and enter recursive main loop
	void showAndBlock();
	
	// Populate the list of particles
	void populateParticleList();

public:
	
	/**
	 * Display the singleton dialog and return the name of the selected 
	 * particle system, or the empty string if none was selected.
	 */
	static std::string chooseParticle();
	
};

}

#endif /*PARTICLESCHOOSER_H_*/

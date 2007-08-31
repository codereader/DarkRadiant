#ifndef PARTICLESCHOOSER_H_
#define PARTICLESCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeselection.h>

#include <string>
#include <map>

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
	
	// Map of particle names -> GtkTreeIter* for quick selection
	typedef std::map<std::string, GtkTreeIter*> IterMap;
	IterMap _iterMap;
	
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
	void showAndBlock(const std::string& current);
	
	// Populate the list of particles
	void populateParticleList();

public:
	
	/**
	 * Display the singleton dialog and return the name of the selected 
	 * particle system, or the empty string if none was selected.
	 * 
	 * @param currentParticle
	 * The particle name which should be highlighted in the list when the dialog
	 * is first displayed. If this value is left at the default value of "", no
	 * particle will be selected.
	 * 
	 * @returns
	 * The name of the particle selected by the user, or an empty string if the
	 * choice was cancelled or invalid.
	 */
	static std::string chooseParticle(const std::string& currentParticle = "");
	
};

}

#endif /*PARTICLESCHOOSER_H_*/

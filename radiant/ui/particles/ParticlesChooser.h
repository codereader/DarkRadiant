#ifndef PARTICLESCHOOSER_H_
#define PARTICLESCHOOSER_H_

#include "gtkutil/window/BlockingTransientWindow.h"

#include "iradiant.h"
#include <string>
#include <map>

#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

namespace ui
{

class ParticlesChooser;
typedef boost::shared_ptr<ParticlesChooser> ParticlesChooserPtr;

/**
 * Chooser dialog for selection (and possibly preview) of particle systems.
 */
class ParticlesChooser :
	public gtkutil::BlockingTransientWindow,
	public RadiantEventListener
{
public:
	// Treemodel definition
	struct ListColumns : 
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	typedef std::map<std::string, Gtk::TreeModel::iterator> IterMap;

private:
	ListColumns _columns;

	// Liststore for the main particles list, and its selection object
	Glib::RefPtr<Gtk::ListStore> _particlesList;
	Glib::RefPtr<Gtk::TreeSelection> _selection;
	
	// Last selected particle
	std::string _selectedParticle;
	
	// Map of particle names -> GtkTreeIter* for quick selection
	IterMap _iterMap;
	
private:
	
	// gtkmm callbacks
	void _onOK();
	void _onCancel();
	void _onSelChanged();
	
	// Constructor creates GTK elements
	ParticlesChooser();
	
	/* WIDGET CONSTRUCTION */
	Gtk::Widget& createTreeView();
	Gtk::Widget& createButtons();
	
	// Static instance owner
	static ParticlesChooser& getInstance();

	static ParticlesChooserPtr& getInstancePtr();
	
	// Show the widgets and enter recursive main loop
	void showAndBlock(const std::string& current);
	
	// Populate the list of particles
	void populateParticleList();

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

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

	// RadiantEventListener
	void onRadiantShutdown();
};

}

#endif /*PARTICLESCHOOSER_H_*/

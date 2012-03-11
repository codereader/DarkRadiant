#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/preview/ParticlePreview.h"

#include "iparticles.h"
#include "iradiant.h"
#include <string>
#include <map>

#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

namespace Gtk
{
	class TreeView;
}

namespace ui
{

class ParticlesChooser;
typedef boost::shared_ptr<ParticlesChooser> ParticlesChooserPtr;

/**
 * \brief
 * Chooser dialog for selection and preview of particle systems.
 */
class ParticlesChooser: public gtkutil::BlockingTransientWindow
{
public:
	typedef std::map<std::string, Gtk::TreeModel::iterator> IterMap;

private:

	// Liststore for the main particles list, and its selection object
	Glib::RefPtr<Gtk::ListStore> _particlesList;
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	Gtk::TreeView* _treeView;

	// Last selected particle
	std::string _selectedParticle;

	// Map of particle names -> GtkTreeIter* for quick selection
	IterMap _iterMap;

	// The preview widget
    gtkutil::ParticlePreviewPtr _preview;

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

	void setSelectedParticle(const std::string& particleName);

private:
	void onRadiantShutdown();
	void reloadParticles();

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

	// Override BlockingTransientWindow::_postShow()
	void _postShow();

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

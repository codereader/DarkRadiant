#include "ParticlesChooser.h"
#include "ParticlesVisitor.h"

#include "i18n.h"
#include "imainframe.h"
#include "iparticles.h"

#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/MultiMonitor.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>

namespace ui
{

ParticlesChooser::ParticlesChooser() :
	gtkutil::BlockingTransientWindow(_("Choose particles"), GlobalMainFrame().getTopLevelWindow()),
	_particlesList(Gtk::ListStore::create(_columns)),
	_selectedParticle("")
{
	set_border_width(12);

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();	

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	set_default_size(
		static_cast<int>(rect.get_width() * 0.4f), static_cast<int>(rect.get_height() * 0.6f)
	);

	// Main dialog vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));
	vbox->pack_start(createTreeView(), true, true, 0);
	vbox->pack_end(createButtons(), false, false, 0);
	
	// Add main vbox to dialog
	add(*vbox);
}

// Create the tree view
Gtk::Widget& ParticlesChooser::createTreeView()
{
	Gtk::TreeView* tv = Gtk::manage(new Gtk::TreeView(_particlesList));

	// Single text column
	tv->append_column(*Gtk::manage(new gtkutil::TextColumnmm(_("Particle"), _columns.name, false)));

	// Apply full-text search to the column
	tv->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContainsmm));
	
	// Populate with particle names
	populateParticleList();
	
	// Connect up the selection changed callback
	_selection = tv->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &ParticlesChooser::_onSelChanged));
	
	// Pack into scrolled window and return
	return *Gtk::manage(new gtkutil::ScrolledFramemm(*tv));
}

// Create the buttons panel
Gtk::Widget& ParticlesChooser::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));
	
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlesChooser::_onOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlesChooser::_onCancel));
	
	hbx->pack_end(*okButton, true, true, 0);	
	hbx->pack_end(*cancelButton, true, true, 0);
					   
	return *Gtk::manage(new gtkutil::RightAlignmentmm(*hbx));
}

// Populate the particles list
void ParticlesChooser::populateParticleList()
{
	// Create and use a ParticlesVisitor to populate the list
	ParticlesVisitor visitor(_particlesList, _columns, _iterMap);
	GlobalParticlesManager().forEachParticleDef(visitor);
}

void ParticlesChooser::onRadiantShutdown()
{
	globalOutputStream() << "ParticlesChooser shutting down." << std::endl;

	// Clear the instance pointer
	getInstancePtr().reset();
}

ParticlesChooserPtr& ParticlesChooser::getInstancePtr()
{
	static ParticlesChooserPtr _instancePtr;
	return _instancePtr;
}

// Static instance owner
ParticlesChooser& ParticlesChooser::getInstance()
{
	ParticlesChooserPtr& instancePtr = getInstancePtr();

	if (instancePtr == NULL)
	{
		instancePtr.reset(new ParticlesChooser);

		GlobalRadiant().addEventListener(instancePtr);
	}

	return *instancePtr;
}

// Enter recursive main loop
void ParticlesChooser::showAndBlock(const std::string& current)
{
    _selectedParticle.clear();

    // Highlight the current particle
    IterMap::const_iterator i = _iterMap.find(current);

    if (i != _iterMap.end())
	{
        _selectedParticle = current;
		_selection->select(i->second);
    }
    
    // Show and block
	show();
}

// Choose a particle system
std::string ParticlesChooser::chooseParticle(const std::string& current)
{
	ParticlesChooser& instance = getInstance();
	instance.showAndBlock(current);

	return instance._selectedParticle;
}

void ParticlesChooser::_onDeleteEvent()
{
	// greebo: Clear the selected name on hide
	_selectedParticle.clear();

	hide(); // just hide, don't call base class which might delete this dialog
}

void ParticlesChooser::_onOK()
{
	hide();
}

void ParticlesChooser::_onCancel()
{	
	// Clear the selection before returning
	_selectedParticle.clear();
	
	hide();
}

void ParticlesChooser::_onSelChanged()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		_selectedParticle = Glib::ustring(row[_columns.name]);
	}
}

} // namespace ui

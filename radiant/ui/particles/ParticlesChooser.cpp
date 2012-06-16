#include "ParticlesChooser.h"

#include "i18n.h"
#include "imainframe.h"
#include "iparticles.h"
#include "iuimanager.h"

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

namespace
{
    // Single column list store
    struct ListColumns: public Gtk::TreeModel::ColumnRecord
    {
        Gtk::TreeModelColumn<std::string> name;
        ListColumns() { add(name); }
    };
    ListColumns& COLUMNS() { static ListColumns _instance; return _instance; }
}

ParticlesChooser::ParticlesChooser() :
	gtkutil::BlockingTransientWindow(_("Choose particles"), GlobalMainFrame().getTopLevelWindow()),
	_particlesList(Gtk::ListStore::create(COLUMNS())),
	_selectedParticle(""),
	_preview(new gtkutil::ParticlePreview)
{
	set_border_width(12);

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	int height = static_cast<int>(rect.get_height() * 0.6f);

	set_default_size(
		static_cast<int>(rect.get_width() * 0.4f), height
	);

	// Set the default size of the window
	_preview->setSize(rect.get_width() * 0.2f, height);

	// Main dialog vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	// Create a horizontal box to pack the treeview on the left and the preview on the right
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	hbox->pack_start(createTreeView(), true, true, 0);

	Gtk::VBox* previewBox = Gtk::manage(new Gtk::VBox(false, 0));
	previewBox->pack_start(*_preview, true, true, 0);

	hbox->pack_start(*previewBox, true, true, 0);

	vbox->pack_start(*hbox, true, true, 0);
	vbox->pack_end(createButtons(), false, false, 0);

	// Add main vbox to dialog
	add(*vbox);
}

// Create the tree view
Gtk::Widget& ParticlesChooser::createTreeView()
{
	_treeView = Gtk::manage(new Gtk::TreeView(_particlesList));

	_treeView->set_size_request(300, -1);

	// Single text column
	_treeView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Particle"), COLUMNS().name, false)));

	// Apply full-text search to the column
	_treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Populate with particle names
	populateParticleList();

	// Connect up the selection changed callback
	_selection = _treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &ParticlesChooser::_onSelChanged));

	// Pack into scrolled window and return
	return *Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));
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

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

namespace
{

/**
 * Visitor class to retrieve particle system names and add them to a
 * GtkListStore.
 */
class ParticlesVisitor
{
	// List store to populate
	Glib::RefPtr<Gtk::ListStore> _store;

	// Map of iters for fast lookup
	ParticlesChooser::IterMap& _iterMap;

public:

	/**
	 * Constructor.
	 */
	ParticlesVisitor(const Glib::RefPtr<Gtk::ListStore>& store,
					 ParticlesChooser::IterMap& map)
	: _store(store),
	  _iterMap(map)
	{}

	/**
	 * Functor operator.
	 */
	void operator() (const particles::IParticleDef& def)
	{
		// Add the ".prt" extension to the name fo display in the list
		std::string prtName = def.getName() + ".prt";

		// Add the Def name to the list store
		Gtk::TreeModel::iterator iter = _store->append();

		Gtk::TreeModel::Row row = *iter;
		row[COLUMNS().name] = prtName;

		// Save the iter in the iter map
		_iterMap.insert(ParticlesChooser::IterMap::value_type(prtName, iter));
	}
};

} // namespace


// Populate the particles list
void ParticlesChooser::populateParticleList()
{
	_iterMap.clear();
	_particlesList->clear();

	// Create and use a ParticlesVisitor to populate the list
	ParticlesVisitor visitor(_particlesList, _iterMap);
	GlobalParticlesManager().forEachParticleDef(visitor);
}

void ParticlesChooser::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_preview->initialisePreview();

	// Call the base class, will enter main loop
	BlockingTransientWindow::_postShow();
}

void ParticlesChooser::onRadiantShutdown()
{
	rMessage() << "ParticlesChooser shutting down." << std::endl;

	_preview.reset();

	// Clear the instance pointer
	getInstancePtr().reset();
}

void ParticlesChooser::reloadParticles()
{
	std::string prevSelection = _selectedParticle;

	populateParticleList();

	// Try to select the previously selected particle again
	setSelectedParticle(prevSelection);
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

	if (!instancePtr)
	{
		instancePtr.reset(new ParticlesChooser);

		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &ParticlesChooser::onRadiantShutdown)
        );
		GlobalParticlesManager().signal_particlesReloaded().connect(
            sigc::mem_fun(*instancePtr, &ParticlesChooser::reloadParticles)
        );
	}

	return *instancePtr;
}

void ParticlesChooser::setSelectedParticle(const std::string& particleName)
{
	// Highlight the current particle
    IterMap::const_iterator i = _iterMap.find(particleName);

    if (i != _iterMap.end())
	{
        _selectedParticle = particleName;
		_selection->select(i->second);

		Gtk::TreeModel::Path path(i->second);

		// Expand the treeview to display the target row
		_treeView->expand_to_path(path);
		// Highlight the target row
		_treeView->set_cursor(path);
		// Make the selected row visible
		_treeView->scroll_to_row(path, 0.3f);
    }
}

// Enter recursive main loop
void ParticlesChooser::showAndBlock(const std::string& current)
{
    _selectedParticle.clear();

    // Highlight the current particle
    setSelectedParticle(current);

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
		_selectedParticle = row[COLUMNS().name];

		_preview->setParticle(_selectedParticle);
	}
}

} // namespace ui

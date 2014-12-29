#include "ParticlesChooser.h"

#include "i18n.h"
#include "imainframe.h"
#include "iparticles.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "ithread.h"

#include "debugging/ScopedDebugTimer.h"

#include "wxutil/TreeView.h"

#include <wx/sizer.h>
#include <functional>

namespace ui
{

namespace
{
    // Single column list store
    struct ListColumns : 
		public wxutil::TreeModel::ColumnRecord
    {
        wxutil::TreeModel::Column name;

		ListColumns() : 
			name(add(wxutil::TreeModel::Column::String)) 
		{}
    };

    ListColumns& COLUMNS() { static ListColumns _instance; return _instance; }
}

ParticlesChooser::ParticlesChooser() :
	DialogBase(_("Choose particles")),
	_particlesList(new wxutil::TreeModel(COLUMNS(), true)),
	_selectedParticle(""),
	_preview(new wxutil::ParticlePreview(this))
{
	// Connect the finish callback to load the treestore
	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED, 
		TreeModelPopulationFinishedHandler(ParticlesChooser::onTreeStorePopulationFinished), NULL, this);

	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(createTreeView(this), 1, wxEXPAND);
	hbox->Add(_preview->getWidget(), 0, wxEXPAND | wxLEFT, 6);

	GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);

	FitToScreen(0.5f, 0.6f);
}

// Create the tree view
wxWindow* ParticlesChooser::createTreeView(wxWindow* parent)
{
	_treeView = wxutil::TreeView::CreateWithModel(parent, _particlesList);
	_treeView->SetSize(300, -1);

	// Single text column
	_treeView->AppendTextColumn(_("Particle"), COLUMNS().name.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Apply full-text search to the column
	_treeView->AddSearchColumn(COLUMNS().name);

	// Start loading particles into the view
	populateParticleList();

	// Connect up the selection changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ParticlesChooser::_onSelChanged), NULL, this);

	return _treeView;
}

/**
 * Visitor class to retrieve particle system names and add them to a
 * treemodel.
 */
class ParticlesChooser::ThreadedParticlesLoader
{
private:
	// List store to populate
	wxutil::TreeModel::Ptr _store;

	wxEvtHandler* _finishedHandler;

public:

	/**
	 * Constructor.
	 */
	ThreadedParticlesLoader(wxEvtHandler* finishedHandler) : 
		_store(new wxutil::TreeModel(COLUMNS(), true)),
		_finishedHandler(finishedHandler)
	{}

	/**
	 * Functor operator.
	 */
	void operator() (const particles::IParticleDef& def)
	{
		// Add the ".prt" extension to the name fo display in the list
		std::string prtName = def.getName() + ".prt";

		// Add the Def name to the list store
		wxutil::TreeModel::Row row = _store->AddItem();

		row[COLUMNS().name] = prtName;
	}

	// The worker function that will execute in the thread
    void run()
    {
        ScopedDebugTimer timer("ThreadedParticlesLoader::run()");

		// Create and use a ParticlesVisitor to populate the list
		GlobalParticlesManager().forEachParticleDef(*this);

		wxutil::TreeModel::PopulationFinishedEvent finishedEvent;
		finishedEvent.SetTreeModel(_store);

		_finishedHandler->AddPendingEvent(finishedEvent);
    }
};

// Populate the particles list
void ParticlesChooser::populateParticleList()
{
	_particlesLoader.reset(new ThreadedParticlesLoader(this));
	
	_particlesList->Clear();

	wxutil::TreeModel::Row row = _particlesList->AddItem();

	row[COLUMNS().name] = "Loading...";
	row.SendItemAdded();
	
	GlobalRadiant().getThreadManager().execute(
		std::bind(&ThreadedParticlesLoader::run, _particlesLoader.get())
    );
}

void ParticlesChooser::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
	_particlesList = ev.GetTreeModel();
    
	// Replace the existing model with the new one
	_treeView->AssociateModel(_particlesList.get());

	// Trigger auto-size calculation
	_particlesList->ItemChanged(_particlesList->GetRoot());

    // Pre-select the given class if requested 
    if (!_preSelectParticle.empty())
    {
        setSelectedParticle(_preSelectParticle);
    }
}

void ParticlesChooser::onRadiantShutdown()
{
	rMessage() << "ParticlesChooser shutting down." << std::endl;

	_preview.reset();

	 // Destroy the window
	SendDestroyEvent();
    getInstancePtr().reset();
}

void ParticlesChooser::reloadParticles()
{
	// Try to select the previously selected particle again
	_preSelectParticle = _selectedParticle;

	populateParticleList();
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
	wxDataViewItem item = _particlesList->FindString(particleName, COLUMNS().name);

	if (item.IsOk())
	{
		_treeView->Select(item);
		_treeView->EnsureVisible(item);

		_preSelectParticle.clear();

		return;
	}
}

// Choose a particle system
std::string ParticlesChooser::ChooseParticle(const std::string& current)
{
	ParticlesChooser& instance = getInstance();
	
	instance._selectedParticle.clear();

	// Try to select the particle right away (will clear _preSelectParticle if successful)
	instance._preSelectParticle = current;
	instance.setSelectedParticle(current);

	int returnCode = instance.ShowModal();

	// Don't destroy the instance, just hide it
	instance.Hide();

	return returnCode == wxID_OK ? instance._selectedParticle : "";
}

void ParticlesChooser::_onSelChanged(wxDataViewEvent& ev)
{
	// Get the selection and store it
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_particlesList);
		_selectedParticle = row[COLUMNS().name];

		_preview->setParticle(_selectedParticle);
	}
}

} // namespace ui

#include "ParticlesChooser.h"

#include "i18n.h"
#include "iparticles.h"
#include "iradiant.h"
#include "ifavourites.h"

#include "debugging/ScopedDebugTimer.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

#include <wx/sizer.h>
#include <functional>

namespace ui
{

ParticlesChooser::ParticlesChooser() :
	DialogBase(_("Choose Particle")),
	_selectedParticle(""),
	_preview(new wxutil::ParticlePreview(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* treeVbox = new wxBoxSizer(wxVERTICAL);

    auto* treeView = createTreeView(this);
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(this, treeView);

    treeVbox->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    treeVbox->Add(treeView, 1, wxEXPAND);

	hbox->Add(treeVbox, 1, wxEXPAND);
	hbox->Add(_preview->getWidget(), 0, wxEXPAND | wxLEFT, 6);

	GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);

	FitToScreen(0.5f, 0.6f);

    GlobalParticlesManager().signal_particlesReloaded().connect(
        sigc::mem_fun(this, &ParticlesChooser::reloadParticles)
    );
}

// Create the tree view
wxutil::ResourceTreeView* ParticlesChooser::createTreeView(wxWindow* parent)
{
	_treeView = new wxutil::ResourceTreeView(parent, _columns, wxDV_NO_HEADER);
	_treeView->SetSize(300, -1);

	_treeView->AppendIconTextColumn(_("Particle"), _columns.iconAndName.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Apply full-text search to the column
	_treeView->AddSearchColumn(_columns.leafName);
    _treeView->EnableFavouriteManagement(decl::Type::Particle);

	// Start loading particles into the view
	populateParticleList();

	// Connect up the selection changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ParticlesChooser::_onSelChanged, this);

	return _treeView;
}

/**
 * Visitor class to retrieve particle system names and add them to a
 * treemodel.
 */
class ThreadedParticlesLoader final :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const wxutil::ResourceTreeView::Columns& _columns;

    std::set<std::string> _favourites;

public:
	ThreadedParticlesLoader(const wxutil::ResourceTreeView::Columns& columns) : 
        ThreadedResourceTreePopulator(columns),
        _columns(columns)
	{
        // Get the list of favourites
        _favourites = GlobalFavouritesManager().getFavourites(decl::Type::Particle);
    }

	~ThreadedParticlesLoader()
	{
        EnsureStopped();
	}

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
	{
        ScopedDebugTimer timer("ThreadedParticlesLoader::run()");

		// Create and use a ParticlesVisitor to populate the list
        GlobalParticlesManager().forEachParticleDef([&](const particles::IParticleDef& def)
        {
            ThrowIfCancellationRequested();

            // Add the ".prt" extension to the name fo display in the list
            std::string prtName = def.getName() + ".prt";

            // Add the Def name to the list store
            wxutil::TreeModel::Row row = model->AddItem();

            bool isFavourite = _favourites.count(prtName) > 0;

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(prtName));
            row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
            row[_columns.fullName] = prtName;
            row[_columns.leafName] = prtName;
            row[_columns.isFolder] = false;
            row[_columns.isFavourite] = isFavourite;

            row.SendItemAdded();
        });
    }
};

// Populate the particles list
void ParticlesChooser::populateParticleList()
{
    _treeView->Populate(std::make_shared<ThreadedParticlesLoader>(_columns));
}

void ParticlesChooser::reloadParticles()
{
	populateParticleList();
}

void ParticlesChooser::setSelectedParticle(const std::string& particleName)
{
    _treeView->SetSelectedFullname(particleName);
}

// Choose a particle system
std::string ParticlesChooser::ChooseParticle(const std::string& current)
{
    auto* dialog = new ParticlesChooser();
	
	dialog->setSelectedParticle(current);

	auto returnValue = dialog->ShowModal() == wxID_OK ? dialog->_selectedParticle : "";

    dialog->Destroy();

    return returnValue;
}

void ParticlesChooser::_onSelChanged(wxDataViewEvent& ev)
{
	// Get the selection and store it
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_treeView->GetTreeModel());
		_selectedParticle = row[_columns.leafName];

		_preview->setParticle(_selectedParticle);
	}
}

} // namespace ui

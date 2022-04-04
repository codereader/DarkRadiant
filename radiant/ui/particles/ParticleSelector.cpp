#include "ParticleSelector.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "ThreadedParticlesLoader.h"

namespace ui
{

ParticleSelector::ParticleSelector(wxWindow* parent) :
    wxPanel(parent, wxID_ANY),
    _preview(new wxutil::ParticlePreview(this))
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Tree view plus toolbar
    auto* treeView = createTreeView(this);
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(this, treeView);

    auto treeVbox = new wxBoxSizer(wxVERTICAL);
    treeVbox->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    treeVbox->Add(treeView, 1, wxEXPAND);

    // Treeview to the left, preview to the right
    auto hbox = new wxBoxSizer(wxHORIZONTAL);

    hbox->Add(treeVbox, 1, wxEXPAND);
    hbox->Add(_preview->getWidget(), 0, wxEXPAND | wxLEFT, 6);

    GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 12);
    
    GlobalParticlesManager().signal_particlesReloaded().connect(
        sigc::mem_fun(this, &ParticleSelector::reloadParticles)
    );
}

wxutil::ResourceTreeView* ParticleSelector::createTreeView(wxWindow* parent)
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
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ParticleSelector::_onSelChanged, this);

    return _treeView;
}

void ParticleSelector::populateParticleList()
{
    _treeView->Populate(std::make_shared<ThreadedParticlesLoader>(_columns));
}

void ParticleSelector::reloadParticles()
{
    populateParticleList();
}

std::string ParticleSelector::getSelectedParticle()
{
    return _treeView->GetSelectedFullname();
}

void ParticleSelector::setSelectedParticle(const std::string& particleName)
{
    _treeView->SetSelectedFullname(particleName);
}

void ParticleSelector::_onSelChanged(wxDataViewEvent& ev)
{
    // Get the selection and store it
    auto item = _treeView->GetSelection();

    if (item.IsOk())
    {
        wxutil::TreeModel::Row row(item, *_treeView->GetTreeModel());
        _preview->setParticle(row[_columns.leafName]);
    }
}

}

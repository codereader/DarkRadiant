#include "ParticleSelector.h"

#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "ThreadedParticlesLoader.h"

namespace ui
{

ParticleSelector::ParticleSelector(wxWindow* parent) :
    DeclarationSelector(parent, decl::Type::Particle),
    _preview(new wxutil::ParticlePreview(this))
{
    AddPreviewToRightPane(_preview->getWidget());
    
    GlobalParticlesManager().signal_particlesReloaded().connect(
        sigc::mem_fun(this, &ParticleSelector::reloadParticles)
    );

    populateParticleList();
}

void ParticleSelector::populateParticleList()
{
    PopulateTreeView(std::make_shared<ThreadedParticlesLoader>(GetColumns()));
}

void ParticleSelector::reloadParticles()
{
    populateParticleList();
}

std::string ParticleSelector::GetSelectedParticle()
{
    return GetTreeView()->GetSelectedFullname();
}

void ParticleSelector::SetSelectedParticle(const std::string& particleName)
{
    GetTreeView()->SetSelectedFullname(particleName);
}

void ParticleSelector::onTreeViewSelectionChanged()
{
    auto selectedParticle = GetSelectedParticle();

    if (!selectedParticle.empty())
    {
        _preview->setParticle(selectedParticle);
    }
}

}

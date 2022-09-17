#pragma once

#include <sigc++/trackable.h>

#include "../common/DeclarationSelector.h"
#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/preview/ParticlePreview.h"

namespace ui
{

/**
 * \brief
 * Chooser widget for selection and preview of particle systems.
 * Features a resource tree view on the left and a render preview on the right.
 */
class ParticleSelector :
    public DeclarationSelector,
    public sigc::trackable
{
private:
    // The preview widget
    wxutil::ParticlePreviewPtr _preview;

public:
    ParticleSelector(wxWindow* parent);

    std::string GetSelectedParticle();
    void SetSelectedParticle(const std::string& particleName);

private:
    // Populate the list of particles
    void populateParticleList();
    void reloadParticles();
};

}

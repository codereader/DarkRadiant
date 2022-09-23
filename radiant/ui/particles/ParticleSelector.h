#pragma once

#include <sigc++/trackable.h>

#include "wxutil/decl/DeclarationSelector.h"
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
    public wxutil::DeclarationSelector,
    public sigc::trackable
{
private:
    // The preview widget
    wxutil::ParticlePreviewPtr _preview;

public:
    ParticleSelector(wxWindow* parent);

    // Returns the selected particle name, including the .prt extension
    std::string GetSelectedParticle();

    // Set the selected particle (particleName includes the .prt extension)
    void SetSelectedParticle(const std::string& particleName);

    // Populate the list of particles
    void Populate() override;

private:
    void reloadParticles();
};

}

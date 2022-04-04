#pragma once

#include <sigc++/trackable.h>
#include <wx/panel.h>

#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/preview/ParticlePreview.h"

namespace ui
{

/**
 * \brief
 * Chooser widget for selection and preview of particle systems.
 * Features a resource tree view on the left and a render preview on the right.
 */
class ParticleSelector :
    public wxPanel,
    public sigc::trackable
{
private:
    wxutil::ResourceTreeView::Columns _columns;

    // Tree view listing all the particles
    wxutil::ResourceTreeView* _treeView;

    // The preview widget
    wxutil::ParticlePreviewPtr _preview;

public:
    ParticleSelector(wxWindow* parent);

    std::string getSelectedParticle();
    void setSelectedParticle(const std::string& particleName);

private:
    wxutil::ResourceTreeView* createTreeView(wxWindow* parent);

    // Populate the list of particles
    void populateParticleList();
    void reloadParticles();

    void _onSelChanged(wxDataViewEvent& ev);
};

}

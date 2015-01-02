#pragma once

#include "RenderPreview.h"

#include <wx/toolbar.h>

#include "iparticles.h"
#include "iparticlenode.h"
#include "imap.h"

#include <string>
#include <map>

namespace wxutil
{

/// RenderPreview widget for particle systems
class ParticlePreview :
    public RenderPreview
{
private:
    wxToolBarToolBase* _showAxesButton;
    wxToolBarToolBase* _showWireFrameButton;
    wxToolBarToolBase* _automaticLoopButton;
	wxToolBarToolBase* _reloadButton;

    scene::IMapRootNodePtr _rootNode;

    // A particle is attached to a parent entity
    scene::INodePtr _entity;

    // Current particle node to display
    particles::IParticleNodePtr _particleNode;

    std::string _lastParticle;

public:

    /// Construct a ParticlePreview widget.
    ParticlePreview(wxWindow* parent);

	~ParticlePreview();

    /**
     * Set the widget to display the given particle. If the particle name is the
     * empty string, the widget will release the currently displayed one.
     *
     * @param
     * String name of the particle to display.
     */
    void setParticle(const std::string& particle);

    /**
     * Get the model from the widget, in order to display properties about it.
     */
    particles::IParticleDefPtr getParticle()
    {
        return _particleNode ? _particleNode->getParticle()->getParticleDef()
                             : particles::IParticleDefPtr();
    }

protected:

    // Creates parent entity etc.
    void setupSceneGraph();

    AABB getSceneBounds();

    bool onPreRender();
    void onPostRender();

private:
    void drawAxes();
    void drawDebugInfo();

	void onToolItemClickRefresh(wxCommandEvent& ev);
};
typedef std::shared_ptr<ParticlePreview> ParticlePreviewPtr;

} // namespace

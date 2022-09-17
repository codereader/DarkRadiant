#include "ParticlePreview.h"

#include "ui/ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "iparticles.h"
#include "iparticlestage.h"
#include "i18n.h"

#include "scene/Node.h"
#include "scene/BasicRootNode.h"
#include "entitylib.h"

#include "string/string.h"
#include "wxutil/GLWidget.h"
#include "wxutil/dialog/MessageBox.h"

#include "../Bitmap.h"

#include <fmt/format.h>
#include "string/predicate.h"

namespace wxutil
{

namespace
{
    const char* const FUNC_EMITTER_CLASS = "func_emitter";

	enum ToolItems
	{
		TOOL_SHOW_AXES = 100,
		TOOL_SHOW_WIREFRAME,
		TOOL_REFRESH,
		TOOL_AUTO_LOOP
	};
}

// Construct the widgets

ParticlePreview::ParticlePreview(wxWindow* parent) :
	RenderPreview(parent, true)
{
    // Add one additional toolbar for particle-related stuff
	wxToolBar* toolbar = new wxToolBar(_mainPanel, wxID_ANY);
	toolbar->SetToolBitmapSize(wxSize(24, 24));

	_showAxesButton = toolbar->AddCheckTool(TOOL_SHOW_AXES, "", 
		wxutil::GetLocalBitmap("axes.png", wxART_TOOLBAR));
	_showAxesButton->SetShortHelp(_("Show coordinate axes"));
	toolbar->Connect(_showAxesButton->GetId(), wxEVT_TOOL, 
		wxCommandEventHandler(ParticlePreview::onToolItemClickRefresh), NULL, this);

	_showWireFrameButton = toolbar->AddCheckTool(TOOL_SHOW_WIREFRAME, "", 
		wxutil::GetLocalBitmap("wireframe.png", wxART_TOOLBAR));
	_showWireFrameButton->SetShortHelp(_("Show wireframe"));
	toolbar->Connect(_showWireFrameButton->GetId(), wxEVT_TOOL, 
		wxCommandEventHandler(ParticlePreview::onToolItemClickRefresh), NULL, this);

	_automaticLoopButton = toolbar->AddCheckTool(TOOL_AUTO_LOOP, _("Auto Loop"), 
		wxutil::GetLocalBitmap("loop.png", wxART_TOOLBAR));
	_automaticLoopButton->SetShortHelp(_("Auto Loop"));

	_reloadButton = toolbar->AddTool(TOOL_REFRESH, "", 
		wxutil::GetLocalBitmap("refresh.png", wxART_TOOLBAR));
    _reloadButton->SetShortHelp(_("Reload Particle Defs"));
    IEventPtr ev = GlobalEventManager().findEvent("ReloadDecls");
	ev->connectToolItem(_reloadButton);

	toolbar->Realize();

    addToolbar(toolbar);
}

ParticlePreview::~ParticlePreview()
{
	IEventPtr ev = GlobalEventManager().findEvent("ReloadDecls");
	ev->disconnectToolItem(_reloadButton);
}

wxWindow* ParticlePreview::GetPreviewWidget()
{
    return _mainPanel;
}

void ParticlePreview::ClearPreview()
{
    SetPreviewDeclName({});
}

void ParticlePreview::SetPreviewDeclName(const std::string& declName)
{
    std::string nameClean = declName;

    if (string::ends_with(nameClean, ".prt"))
    {
        nameClean = nameClean.substr(0, nameClean.length() - 4);
    }

    // If the model name is empty, release the model
    if (nameClean.empty())
    {
        if (_particleNode)
        {
            _entity->removeChildNode(_particleNode);
        }

        _particleNode.reset();
        _lastParticle = "";
        stopPlayback();
        return;
    }

    // Set up the scene
    if (!_entity)
    {
        setupSceneGraph();
    }

	if (!_entity) return; // FUNC_EMITTER_CLASS not found 

    if (_particleNode)
    {
        _entity->removeChildNode(_particleNode);
    }

    // Construct the particle emitter node
    _particleNode = GlobalParticlesManager().createParticleNode(nameClean);

    if (_particleNode && _lastParticle != nameClean)
    {
        _entity->addChildNode(_particleNode);

        // Reset preview time
        stopPlayback();

        // Call update(0) once to enable the bounds calculation
        _particleNode->getParticle()->update(_modelView, _particleNode->localToWorld(), _entity->getRenderEntity());

        // Reset the model rotation
        resetModelRotation();

        // Use particle AABB to adjust camera distance
        const AABB& particleBounds = _particleNode->getParticle()->getBounds();

        if (particleBounds.isValid())
        {
            // Reset the default view, facing down to the model from diagonally above the bounding box
            double distance = particleBounds.getRadius() * 2.0f;

            setViewOrigin(Vector3(1, 1, 1) * distance);
        }
        else
        {
            // Bounds not valid, fall back to default
            setViewOrigin(Vector3(1, 1, 1) * 40.0f);
        }

        setViewAngles(Vector3(34, 135, 0));

        _lastParticle = nameClean;

        // Start playback when switching particles
        startPlayback();
    }

    // Redraw
    queueDraw();
}

void ParticlePreview::setupSceneGraph()
{
    RenderPreview::setupSceneGraph();

	try
	{
        _rootNode = std::make_shared<scene::BasicRootNode>();
        
		_entity = GlobalEntityModule().createEntity(
			GlobalEntityClassManager().findClass(FUNC_EMITTER_CLASS));

        _rootNode->addChildNode(_entity);

		_entity->enable(scene::Node::eHidden);

		// This entity is acting as our root node in the scene
        getScene()->setRoot(_rootNode);
	}
	catch (std::runtime_error&)
	{
		wxutil::Messagebox::ShowError(fmt::format(_("Unable to setup the preview,\n"
			"could not find the entity class {0}"), FUNC_EMITTER_CLASS));
	}
}

AABB ParticlePreview::getSceneBounds()
{
    if (!_particleNode)
    {
        return RenderPreview::getSceneBounds();
    }

    return _particleNode->getParticle()->getBounds();
}

Vector3 ParticlePreview::getGridOrigin()
{
    return Vector3(0, 0, 0);
}

bool ParticlePreview::onPreRender()
{
    return _particleNode != NULL;
}

void ParticlePreview::onPostRender()
{
	if (_showWireFrameButton->IsToggled())
    {
        renderWireFrame();
    }

    // Draw coordinate axes for better orientation
    if (_showAxesButton->IsToggled())
    {
        drawAxes();
    }

    const particles::IParticleDef::Ptr& def = _particleNode->getParticle()->getParticleDef();

    // Calculate the total time of the particles
    std::size_t totalTimeMsec = 0;

    for (std::size_t i = 0; i < def->getNumStages(); ++i)
    {
        const auto& stage = def->getStage(i);

        // For ever-repeating stages, set stuff to INT_MAX and break
        if (stage->getCycles() == 0)
        {
            totalTimeMsec = INT_MAX;
            break;
        }

        totalTimeMsec += static_cast<int>(stage->getCycleMsec() * stage->getCycles());
    }

    // Update the sensitivity of the auto-loop button
    if (totalTimeMsec < INT_MAX)
    {
		_automaticLoopButton->GetToolBar()->EnableTool(TOOL_AUTO_LOOP, true);

        // Auto-Loop is possible, check if we should reset the time
		if (_automaticLoopButton->IsToggled() && _renderSystem->getTime() > totalTimeMsec)
        {
            _renderSystem->setTime(0);
        }
    }
    else
    {
		_automaticLoopButton->GetToolBar()->EnableTool(TOOL_AUTO_LOOP, false);
    }
}

void ParticlePreview::onModelRotationChanged()
{
    if (_entity)
    {
        // Update the model rotation on the entity
        std::ostringstream value;
        value << _modelRotation.xx() << ' '
            << _modelRotation.xy() << ' '
            << _modelRotation.xz() << ' '
            << _modelRotation.yx() << ' '
            << _modelRotation.yy() << ' '
            << _modelRotation.yz() << ' '
            << _modelRotation.zx() << ' '
            << _modelRotation.zy() << ' '
            << _modelRotation.zz();

        Node_getEntity(_entity)->setKeyValue("rotation", value.str());
    }
}

void ParticlePreview::drawAxes()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glLineWidth(2);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBegin(GL_LINES);

    glColor4f(1,0,0,0.6f);
    glVertex3f(0,0,0);
    glVertex3f(5,0,0);

    glColor4f(0,1,0,0.6f);
    glVertex3f(0,0,0);
    glVertex3f(0,5,0);

    glColor4f(0,0,1,0.6f);
    glVertex3f(0,0,0);
    glVertex3f(0,0,5);

    glEnd();
}

void ParticlePreview::onToolItemClickRefresh(wxCommandEvent& ev)
{
	queueDraw();
}

} // namespace ui

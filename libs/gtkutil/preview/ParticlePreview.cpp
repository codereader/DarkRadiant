#include "ParticlePreview.h"

#include "iuimanager.h"
#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "iparticles.h"
#include "iparticlestage.h"
#include "i18n.h"

#include "scene/Node.h"
#include "entitylib.h"

#include "string/string.h"
#include "gtkutil/GLWidget.h"

#include <gtkmm/box.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/image.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/stock.h>

#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace gtkutil
{

namespace
{
    const char* const FUNC_EMITTER_CLASS = "func_emitter";
}

// Construct the widgets

ParticlePreview::ParticlePreview()
{
    // Add one additional toolbar for particle-related stuff
    Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
    toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);

    _showAxesButton = Gtk::manage(new Gtk::ToggleToolButton);
    _showAxesButton->signal_toggled().connect(
        sigc::mem_fun(this, &ParticlePreview::queue_draw)
    );
    _showAxesButton->set_icon_widget(*Gtk::manage(new Gtk::Image(
        GlobalUIManager().getLocalPixbufWithMask("axes.png"))));
    _showAxesButton->set_tooltip_text(_("Show coordinate axes"));

    Gtk::ToolButton* reloadButton = Gtk::manage(new Gtk::ToolButton);
    reloadButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::REFRESH, Gtk::ICON_SIZE_MENU)));
    reloadButton->set_tooltip_text(_("Reload Particle Defs"));
    IEventPtr ev = GlobalEventManager().findEvent("ReloadParticles");
    ev->connectWidget(reloadButton);

    _showWireFrameButton = Gtk::manage(new Gtk::ToggleToolButton);
    _showWireFrameButton->set_icon_widget(*Gtk::manage(new Gtk::Image(
        GlobalUIManager().getLocalPixbufWithMask("wireframe.png"))));
    _showWireFrameButton->set_tooltip_text(_("Show wireframe"));
    _showWireFrameButton->signal_toggled().connect(
        sigc::mem_fun(this, &ParticlePreview::queue_draw)
    );

    _automaticLoopButton = Gtk::manage(new Gtk::ToggleToolButton(_("Auto Loop")));
    _automaticLoopButton->set_tooltip_text(_("Auto Loop"));

    toolbar->insert(*_showAxesButton, 0);
    toolbar->insert(*_showWireFrameButton, 0);
    toolbar->insert(*_automaticLoopButton, 0);
    toolbar->insert(*reloadButton, 0);

    addToolbar(*toolbar);
}

void ParticlePreview::setParticle(const std::string& name)
{
    std::string nameClean = name;

    if (boost::algorithm::ends_with(nameClean, ".prt"))
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
        stopPlayback();
        return;
    }

    // Set up the scene
    if (!_entity)
    {
        setupSceneGraph();
    }

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

        // Reset the rotation to the default one
        _rotation = Matrix4::getRotation(Vector3(0,-1,0), Vector3(0,-0.3f,1));
        _rotation.multiplyBy(Matrix4::getRotation(Vector3(0,1,0), Vector3(1,-1,0)));

        // Call update(0) once to enable the bounds calculation
        _particleNode->getParticle()->update(_rotation);

        // Use particle AABB to adjust camera distance
        const AABB& particleBounds = _particleNode->getParticle()->getBounds();

        if (particleBounds.isValid())
        {
            _camDist = -2.0f * static_cast<float>(particleBounds.getRadius());
        }
        else
        {
            // Bounds not valid, fall back to default
            _camDist = -40.0f;
        }

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

    _entity = GlobalEntityCreator().createEntity(
        GlobalEntityClassManager().findClass(FUNC_EMITTER_CLASS));

    _entity->enable(scene::Node::eHidden);

    // This entity is acting as our root node in the scene
    getScene()->setRoot(_entity);
}

AABB ParticlePreview::getSceneBounds()
{
    if (!_particleNode)
    {
        return RenderPreview::getSceneBounds();
    }

    return _particleNode->getParticle()->getBounds();
}

bool ParticlePreview::onPreRender()
{
    return _particleNode != NULL;
}

void ParticlePreview::onPostRender()
{
    if (_showWireFrameButton->get_active())
    {
        renderWireFrame();
    }

    // Draw coordinate axes for better orientation
    if (_showAxesButton->get_active())
    {
        drawAxes();
    }

    const particles::IParticleDefPtr& def = _particleNode->getParticle()->getParticleDef();

    // Calculate the total time of the particles
    std::size_t totalTimeMsec = 0;

    for (std::size_t i = 0; i < def->getNumStages(); ++i)
    {
        const particles::IStageDef& stage = def->getStage(i);

        // For ever-repeating stages, set stuff to INT_MAX and break
        if (stage.getCycles() == 0)
        {
            totalTimeMsec = INT_MAX;
            break;
        }

        totalTimeMsec += static_cast<int>(stage.getCycleMsec() * stage.getCycles());
    }

    // Update the sensitivity of the auto-loop button
    if (totalTimeMsec < INT_MAX)
    {
        _automaticLoopButton->set_sensitive(true);

        // Auto-Loop is possible, check if we should reset the time
        if (_automaticLoopButton->get_active() && _renderSystem->getTime() > totalTimeMsec)
        {
            _renderSystem->setTime(0);
        }
    }
    else
    {
        _automaticLoopButton->set_sensitive(false);
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

} // namespace ui

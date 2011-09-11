#pragma once

#include "gtkutil/preview/RenderPreview.h"

#include "iparticles.h"
#include "iparticlenode.h"
#include "iparticlepreview.h"

#include <string>
#include <map>

namespace ui
{

class ParticlePreview :
	public IParticlePreview,
	public gtkutil::RenderPreview
{
private:
	Gtk::ToggleToolButton* _showAxesButton;
	Gtk::ToggleToolButton* _showWireFrameButton;
	Gtk::ToggleToolButton* _automaticLoopButton;

	// A particle is attached to a paren entity
	scene::INodePtr _entity;

	// Current particle node to display
	particles::IParticleNodePtr _particle;
	scene::INodePtr _particleNode;

	std::string _lastParticle;

public:
	/** Construct a ParticlePreview widget.
	 */
	ParticlePreview();

	// IParticlePreview implementation, wrapping to base
	void setSize(int width, int height)
	{
		RenderPreview::setSize(width, height);
	}

	void initialisePreview()
	{
		RenderPreview::initialisePreview();
	}

	/**
	 * Set the widget to display the given particle. If the particle name is the
	 * empty string, the widget will release the currently displayed one.
	 *
	 * @param
	 * String name of the particle to display.
	 */
	void setParticle(const std::string& particle);

	// Retrieve the widget to pack this element into a parent container
	Gtk::Widget* getWidget() 
	{
		return this;
	}

	/**
	 * Get the model from the widget, in order to display properties about it.
	 */
	particles::IParticleDefPtr getParticle()
	{
		return _particle ? _particle->getParticle()->getParticleDef() : particles::IParticleDefPtr();
	}

protected:
	void onToggleAxes();

	// Creates parent entity etc.
	void setupSceneGraph();

	AABB getSceneBounds();

	bool onPreRender();
	void onPostRender();

private:
	void drawAxes();
	void drawDebugInfo();
};
typedef boost::shared_ptr<ParticlePreview> ParticlePreviewPtr;

} // namespace

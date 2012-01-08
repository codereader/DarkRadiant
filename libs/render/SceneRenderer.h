#pragma once

#include "irender.h"
#include "iscenegraph.h"

namespace render
{

/**
 * Simple implementation of a scene::Walker passing visited nodes
 * on to the attached RenderableCollector. Suitable for use in the
 * scene::Graph::foreach*() methods.
 */
class SceneRenderer :
	public scene::Graph::Walker
{
private:
	// The collector which is sorting our renderables
	RenderableCollector& _collector;

	// The view we're using for culling
	const VolumeTest& _volume;

public:
	SceneRenderer(RenderableCollector& collector, const VolumeTest& volume) :
		_collector(collector),
		_volume(volume)
	{}

	// Render function, instructs the Renderable object to submit its geometry
	// to the contained RenderableCollector.
	void render(const Renderable& renderable) const
	{
	    if (_collector.supportsFullMaterials())
			renderable.renderSolid(_collector, _volume);
        else
			renderable.renderWireframe(_collector, _volume);
	}

	// scene::Graph::Walker implementation, tells each node to submit its OpenGLRenderables
	bool visit(const scene::INodePtr& node)
	{
		_collector.PushState();

		node->viewChanged();

		render(*node);

		_collector.PopState();

		return true;
	}
};

} // namespace render

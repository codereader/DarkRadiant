#pragma once

#include "irender.h"
#include "iscenegraph.h"

namespace render
{

/// A scenegraph walker that passes nodes to its contained RenderableCollector
class SceneRenderWalker :
	public scene::Graph::Walker
{
	// The collector which is sorting our renderables
	RenderableCollector& _collector;

	// The view we're using for culling
	const VolumeTest& _volume;

private:
	void render(const Renderable& renderable) const
	{
	    if (_collector.supportsFullMaterials())
			renderable.renderSolid(_collector, _volume);
        else
			renderable.renderWireframe(_collector, _volume);
	}

public:

    /// Initialise with a RenderableCollector to populate and a view volume
	SceneRenderWalker(RenderableCollector& collector, const VolumeTest& volume) :
		_collector(collector),
		_volume(volume)
	{}

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

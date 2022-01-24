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
	IRenderableCollector& _collector;

	// The view we're using for culling
	const VolumeTest& _volume;

public:

    /// Initialise with a RenderableCollector to populate and a view volume
	SceneRenderWalker(IRenderableCollector& collector, const VolumeTest& volume) :
		_collector(collector),
		_volume(volume)
	{}

	// scene::Graph::Walker implementation
	bool visit(const scene::INodePtr& node) override
	{
        node->onPreRender(_volume);
		return true;
	}
};

} // namespace render

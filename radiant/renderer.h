#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "itextstream.h"
#include "irender.h"
#include "irenderable.h"
#include "render/frontend/RenderHighlighted.h"

namespace render
{

class TestVisitor :
	public scene::Graph::Walker
{
public:
	TestVisitor() :
		_renderables(0),
		_nonRenderables(0)
	{
	}

	std::size_t _renderables;
	std::size_t _nonRenderables;

	bool visit(const scene::INodePtr& node)
	{
		bool isRenderable = boost::dynamic_pointer_cast<Renderable>(node);

		if (isRenderable)
		{
			_renderables++;
		}
		else
		{
			_nonRenderables++;
		}

		return true;
	}
};

/**
 * Scene render function. Uses the visibility walkers to traverse the scene
 * graph and submit all visible objects to the provided RenderableCollector.
 */
inline void collectRenderablesInScene(RenderableCollector& collector, const VolumeTest& volume)
{
	TestVisitor visitor;
	GlobalSceneGraph().foreachVisibleNodeInVolume(volume, visitor);

	globalOutputStream() << "Renderables: " << visitor._renderables << ", Non-renderables: " << visitor._nonRenderables << std::endl;

	// Instantiate a new walker class
	RenderHighlighted renderHighlightWalker(collector, volume);

	// Submit renderables from scene graph
	GlobalSceneGraph().foreachVisibleNodeInVolume(volume, renderHighlightWalker);

	// Submit renderables directly attached to the ShaderCache
	RenderHighlighted walker(collector, volume);
	GlobalRenderSystem().forEachRenderable(walker.getRenderableCallback());
}

} // namespace render

#endif /* _RENDERER_H_ */

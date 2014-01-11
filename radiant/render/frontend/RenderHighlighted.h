#pragma once

#include "iselection.h"
#include "ientity.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include <boost/bind.hpp>

namespace render
{

class RenderHighlighted :
	public scene::Graph::Walker
{
private:
	// The collector which is sorting our renderables
	RenderableCollector& _collector;

	// The view we're using for culling
	const VolumeTest& _volume;

public:
	RenderHighlighted(RenderableCollector& collector, const VolumeTest& volume) :
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

	RenderableCallback getRenderableCallback()
	{
		return boost::bind(&RenderHighlighted::render, this, _1);
	}

	// scene::Graph::Walker implementation, tells each node to submit its OpenGLRenderables
	bool visit(const scene::INodePtr& node)
	{
		_collector.PushState();

		// greebo: Fix for primitive nodes: as we don't traverse the scenegraph nodes
		// top-down anymore, we need to set the shader state of our parent entity ourselves.
		// Otherwise we're in for NULL-states when rendering worldspawn brushes.
		scene::INodePtr parent = node->getParent();

		Entity* entity = Node_getEntity(parent);

		if (entity != NULL)
		{
			const IRenderEntityPtr& renderEntity = node->getRenderEntity();

			assert(renderEntity);

			if (renderEntity)
			{
				_collector.SetState(renderEntity->getWireShader(), RenderableCollector::eWireframeOnly);
			}
		}

		node->viewChanged();

		if (node->isHighlighted() || (parent != NULL && parent->isHighlighted()))
		{
			if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
			{
				_collector.highlightFaces(true);
			}
			else
			{
				node->renderComponents(_collector, _volume);
			}

			_collector.highlightPrimitives(true);
		}

		render(*node);

		_collector.PopState();

		return true;
	}

	/**
	 * Scene render function. Uses the visibility walkers to traverse the scene
	 * graph and submit all visible objects to the provided RenderableCollector.
	 */
	static void collectRenderablesInScene(RenderableCollector& collector, const VolumeTest& volume)
	{
		// Instantiate a new walker class
		RenderHighlighted renderHighlightWalker(collector, volume);

		// Submit renderables from scene graph
		GlobalSceneGraph().foreachVisibleNodeInVolume(volume, renderHighlightWalker);

		// Submit renderables directly attached to the ShaderCache
		RenderHighlighted walker(collector, volume);
		GlobalRenderSystem().forEachRenderable(walker.getRenderableCallback());
	}
};

} // namespace

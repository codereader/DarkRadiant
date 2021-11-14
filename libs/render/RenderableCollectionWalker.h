#pragma once

#include "iselection.h"
#include "imapmerge.h"
#include "ientity.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include <functional>
#include "render/RenderableCollectorBase.h"

namespace render
{

/**
 * \brief
 * Scenegraph walker class that finds all renderable objects and adds them to a
 * contained RenderableCollector.
 *
 * Also provides support for highlighting selected objects by activating the
 * RenderableCollector's "highlight" flags based on the renderable object's
 * selection state.
 */
class RenderableCollectionWalker :
    public scene::Graph::Walker
{
private:
    // The collector which is sorting our renderables
    RenderableCollectorBase& _collector;

    // The view we're using for culling
    const VolumeTest& _volume;

    // Construct with the collector to process the nodes
    RenderableCollectionWalker(RenderableCollectorBase& collector, const VolumeTest& volume) :
		_collector(collector), 
		_volume(volume)
    {}

public:
    // scene::Graph::Walker implementation
    bool visit(const scene::INodePtr& node) override
    {
        _collector.processNode(node, _volume);
        return true;
    }

    /**
     * \brief
     * Use a RenderableCollectionWalker to find all renderables in the global
     * scenegraph.
     */
    static void CollectRenderablesInScene(RenderableCollectorBase& collector, const VolumeTest& volume)
    {
        // Instantiate a new walker class
        RenderableCollectionWalker renderHighlightWalker(collector, volume);

        // Submit renderables from scene graph
        GlobalSceneGraph().foreachVisibleNodeInVolume(volume, renderHighlightWalker);

        // Submit any renderables that have been directly attached to the RenderSystem
		// without belonging to an actual scene object
        RenderableCollectionWalker walker(collector, volume);
		GlobalRenderSystem().forEachRenderable([&](const Renderable& renderable)
		{
			collector.processRenderable(renderable, volume);
		});
    }
};

} // namespace

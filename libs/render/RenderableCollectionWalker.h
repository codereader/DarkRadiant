#pragma once

#include "iselection.h"
#include "iscenegraph.h"
#include <functional>
#include "render/RenderableCollectorBase.h"

namespace render
{

/**
 * \brief
 * Scenegraph walker class that finds all renderable objects and adds them to a
 * given RenderableCollector.
 */
class RenderableCollectionWalker
{
public:
    /**
     * \brief
     * Use a RenderableCollectionWalker to find all renderables in the global
     * scenegraph.
     */
    static void CollectRenderablesInScene(RenderableCollectorBase& collector, const VolumeTest& volume)
    {
        // Submit renderables from scene graph
        GlobalSceneGraph().foreachVisibleNodeInVolume(volume, [&](const scene::INodePtr& node)
        {
            collector.processNode(node, volume);
            return true;
        });

        // Prepare any renderables that have been directly attached to the RenderSystem
		// without belonging to an actual scene object
		GlobalRenderSystem().forEachRenderable([&](Renderable& renderable)
		{
            renderable.onPreRender(volume);
		});
    }
};

} // namespace

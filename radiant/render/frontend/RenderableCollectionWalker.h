#pragma once

#include "iselection.h"
#include "ientity.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include <functional>

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
    // The collector which is sorting our renderables
    RenderableCollector& _collector;

    // The view we're using for culling
    const VolumeTest& _volume;

private:

    // Construct with RenderableCollector to receive renderables
    RenderableCollectionWalker(RenderableCollector& collector,
                               const VolumeTest& volume)
    : _collector(collector), _volume(volume)
    {}

    void render(const Renderable& renderable) const
    {
        if (_collector.supportsFullMaterials())
            renderable.renderSolid(_collector, _volume);
        else
            renderable.renderWireframe(_collector, _volume);
    }

    RenderableCallback getRenderableCallback()
    {
        return std::bind(&RenderableCollectionWalker::render, this, std::placeholders::_1);
    }

public:

    // scene::Graph::Walker implementation
    bool visit(const scene::INodePtr& node)
    {
        _collector.PushState();

        // greebo: Fix for primitive nodes: as we don't traverse the scenegraph
        // nodes top-down anymore, we need to set the shader state of our
        // parent entity ourselves.  Otherwise we're in for NULL-states when
        // rendering worldspawn brushes.
        scene::INodePtr parent = node->getParent();

        Entity* entity = Node_getEntity(parent);

        if (entity != NULL)
        {
            const IRenderEntity* renderEntity = node->getRenderEntity();

            assert(renderEntity);

            if (renderEntity)
            {
                _collector.SetState(renderEntity->getWireShader(), RenderableCollector::eWireframeOnly);
            }
        }

        node->viewChanged();

		std::size_t highlightFlags = node->getHighlightFlags();

		if (parent)
		{
			highlightFlags |= parent->getHighlightFlags();
		}

        if (highlightFlags & Renderable::Highlight::Selected)
        {
            if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
            {
				_collector.setHighlightFlag(RenderableCollector::Highlight::Faces, true);
            }
            else
            {
                node->renderComponents(_collector, _volume);
            }

            _collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, true);
        }

        render(*node);

        _collector.PopState();

        return true;
    }

    /**
     * \brief
     * Use a RenderableCollectionWalker to find all renderables in the global
     * scenegraph.
     */
    static void collectRenderablesInScene(RenderableCollector& collector,
                                          const VolumeTest& volume)
    {
        // Instantiate a new walker class
        RenderableCollectionWalker renderHighlightWalker(collector, volume);

        // Submit renderables from scene graph
        GlobalSceneGraph().foreachVisibleNodeInVolume(volume,
                                                      renderHighlightWalker);

        // Submit renderables directly attached to the ShaderCache
        RenderableCollectionWalker walker(collector, volume);
        GlobalRenderSystem().forEachRenderable(walker.getRenderableCallback());
    }
};

} // namespace

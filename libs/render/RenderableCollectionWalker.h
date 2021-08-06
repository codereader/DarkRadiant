#pragma once

#include "iselection.h"
#include "imapmerge.h"
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
private:
    // The collector which is sorting our renderables
    RenderableCollector& _collector;

    // The view we're using for culling
    const VolumeTest& _volume;

    // Construct with RenderableCollector to receive renderables
    RenderableCollectionWalker(RenderableCollector& collector, const VolumeTest& volume) : 
		_collector(collector), 
		_volume(volume)
    {}

public:
	void dispatchRenderable(const Renderable& renderable)
	{
		if (_collector.supportsFullMaterials())
		{
			renderable.renderSolid(_collector, _volume);
		}
		else
		{
			renderable.renderWireframe(_collector, _volume);
		}
	}

    // scene::Graph::Walker implementation
    bool visit(const scene::INodePtr& node)
    {
        // greebo: Highlighting propagates to child nodes
        scene::INodePtr parent = node->getParent();

        node->viewChanged();

		std::size_t highlightFlags = node->getHighlightFlags();

        auto nodeType = node->getNodeType();

        // Particle nodes shouldn't inherit the highlight flags from their parent, 
        // as it obstructs the view when their wireframe gets rendered (#5682)
		if (parent && nodeType != scene::INode::Type::Particle)
		{
			highlightFlags |= parent->getHighlightFlags();
		}

        if (nodeType == scene::INode::Type::MergeAction)
        {
            _collector.setHighlightFlag(RenderableCollector::Highlight::MergeAction, true);

            auto mergeActionNode = std::dynamic_pointer_cast<scene::IMergeActionNode>(node);
            assert(mergeActionNode);

            switch (mergeActionNode->getActionType())
            {
            case scene::merge::ActionType::AddChildNode:
            case scene::merge::ActionType::AddEntity:
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionAdd, true);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionChange, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionRemove, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionConflict, false);
                break;

            case scene::merge::ActionType::AddKeyValue:
            case scene::merge::ActionType::ChangeKeyValue:
            case scene::merge::ActionType::RemoveKeyValue:
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionChange, true);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionAdd, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionRemove, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionConflict, false);
                break;

            case scene::merge::ActionType::RemoveChildNode:
            case scene::merge::ActionType::RemoveEntity:
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionRemove, true);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionAdd, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionChange, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionConflict, false);
                break;

            case scene::merge::ActionType::ConflictResolution:
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionConflict, true);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionAdd, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionChange, false);
                _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionRemove, false);
                break;
            }
        }
        else
        {
            _collector.setHighlightFlag(RenderableCollector::Highlight::MergeAction, false);
            _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionAdd, false);
            _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionChange, false);
            _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionRemove, false);
            _collector.setHighlightFlag(RenderableCollector::Highlight::MergeActionConflict, false);
        }

        if (highlightFlags & Renderable::Highlight::Selected)
        {
            if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
            {
				_collector.setHighlightFlag(RenderableCollector::Highlight::Faces, true);
            }
            else
            {
				_collector.setHighlightFlag(RenderableCollector::Highlight::Faces, false);
                node->renderComponents(_collector, _volume);
            }

			_collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, true);

			// Pass on the info about whether we have a group member selected
			if (highlightFlags & Renderable::Highlight::GroupMember)
			{
				_collector.setHighlightFlag(RenderableCollector::Highlight::GroupMember, true);
			}
			else
			{
				_collector.setHighlightFlag(RenderableCollector::Highlight::GroupMember, false);
			}
        }
		else
		{
			_collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, false);
			_collector.setHighlightFlag(RenderableCollector::Highlight::Faces, false);
			_collector.setHighlightFlag(RenderableCollector::Highlight::GroupMember, false);
		}

		dispatchRenderable(*node);

        return true;
    }

    /**
     * \brief
     * Use a RenderableCollectionWalker to find all renderables in the global
     * scenegraph.
     */
    static void CollectRenderablesInScene(RenderableCollector& collector, const VolumeTest& volume)
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
			walker.dispatchRenderable(renderable);
		});
    }
};

} // namespace

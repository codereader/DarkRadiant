#include "MergeActionNode.h"

namespace scene
{

MergeActionNodeBase::MergeActionNodeBase() :
    _syncActionStatus(true)
{}

void MergeActionNodeBase::prepareForMerge()
{
    _syncActionStatus = false;

    // Unhide ourselves before merging
    if (checkStateFlag(eHidden))
    {
        disable(eHidden);
    }
}

INodePtr MergeActionNodeBase::getAffectedNode()
{
    return _affectedNode;
}

void MergeActionNodeBase::clear()
{
    _affectedNode.reset();
}

void MergeActionNodeBase::onInsertIntoScene(IMapRootNode& rootNode)
{
    if (_syncActionStatus)
    {
        foreachMergeAction([&](const merge::IMergeAction::Ptr& action)
        {
            action->activate();
        });

        // The activated actions might have changed the node bounds in some way
        boundsChanged();
    }

    // Don't hide affected nodes, they need to maintain their renderables
    //hideAffectedNodes();

    SelectableNode::onInsertIntoScene(rootNode);
}

void MergeActionNodeBase::onRemoveFromScene(IMapRootNode& rootNode)
{
    SelectableNode::onRemoveFromScene(rootNode);
    
    unhideAffectedNodes();

    if (_syncActionStatus)
    {
        foreachMergeAction([&](const merge::IMergeAction::Ptr& action)
        {
            // Removing an unresolved conflict action from the scene implies rejecting the source change
            auto conflictAction = std::dynamic_pointer_cast<merge::IConflictResolutionAction>(action);
            
            if (conflictAction && conflictAction->getResolution() == merge::ResolutionType::Unresolved)
            {
                conflictAction->setResolution(merge::ResolutionType::RejectSourceChange);
            }

            // Removing any action from the scene means to deactivate it
            action->deactivate();
        });
    }
}

INode::Type MergeActionNodeBase::getNodeType() const
{
    return INode::Type::MergeAction;
}

bool MergeActionNodeBase::supportsStateFlag(unsigned int state) const
{
    if ((state & (eFiltered | eExcluded | eLayered)) != 0)
    {
        return false; // don't allow this node to be hidden
    }

    return Node::supportsStateFlag(state);
}

const AABB& MergeActionNodeBase::localAABB() const
{
    // We report the contained node's world AABB and an identity transform
    return _affectedNode->worldAABB();
}

const Matrix4& MergeActionNodeBase::localToWorld() const
{
    // We report the contained node's world AABB and an identity transform
    static Matrix4 identity = Matrix4::getIdentity();
    return identity;
}

void MergeActionNodeBase::onPreRender(const VolumeTest& volume)
{
    _affectedNode->onPreRender(volume);
    _affectedNode->foreachNode([&](const INodePtr& child)
    {
        child->onPreRender(volume);
        return true;
    });
}

void MergeActionNodeBase::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    _affectedNode->renderHighlights(collector, volume);
    _affectedNode->foreachNode([&](const INodePtr& child)
    {
        child->renderHighlights(collector, volume);
        return true;
    });
}

std::size_t MergeActionNodeBase::getHighlightFlags()
{
    return isSelected() ? Highlight::Selected : Highlight::NoHighlight;
}

void MergeActionNodeBase::testSelect(Selector& selector, SelectionTest& test)
{
    testSelectNode(_affectedNode, selector, test);

    _affectedNode->foreachNode([&](const INodePtr& child)
    {
        testSelectNode(child, selector, test);
        return true;
    });
}

void MergeActionNodeBase::testSelectNode(const INodePtr& node, Selector& selector, SelectionTest& test)
{
    auto selectionTestable = std::dynamic_pointer_cast<SelectionTestable>(node);

    // Regardless of what node we test, it will always be the MergeActionNode that will be selected
    selector.pushSelectable(*this);

    if (selectionTestable)
    {
        selectionTestable->testSelect(selector, test);
    }

    selector.popSelectable();
}

void MergeActionNodeBase::hideAffectedNodes()
{
    // Hide the affected node itself, we're doing the rendering ourselves, recursively
    _affectedNode->enable(Node::eExcluded);

    _affectedNode->foreachNode([&](const INodePtr& child)
    {
        child->enable(Node::eExcluded);
        return true;
    });
}

void MergeActionNodeBase::unhideAffectedNodes()
{
    // Release the excluded state of the contained nodes
    _affectedNode->disable(Node::eExcluded);

    _affectedNode->foreachNode([&](const INodePtr& child)
    {
        child->disable(Node::eExcluded);
        return true;
    });
}

// ------------ KeyValueMergeActionNode ----------------------------

KeyValueMergeActionNode::KeyValueMergeActionNode(const std::vector<merge::IMergeAction::Ptr>& actions) :
    _actions(actions)
{
    assert(!_actions.empty());

    _affectedNode = _actions.front()->getAffectedNode();
    assert(std::find_if(_actions.begin(), _actions.end(),
        [&](const merge::IMergeAction::Ptr& action) { return action->getAffectedNode() != _affectedNode; }) == _actions.end());
}

void KeyValueMergeActionNode::clear()
{
    _actions.clear();
}

merge::ActionType KeyValueMergeActionNode::getActionType() const
{
    // We report the change key value type since we're doing all kinds of key value changes,
    // unless we have an unresolved conflict in our collection
    auto activeConflict = std::find_if(_actions.begin(), _actions.end(), [&](const merge::IMergeAction::Ptr& action) 
    {
        auto conflict = std::dynamic_pointer_cast<merge::IConflictResolutionAction>(action);

        return conflict && conflict->isActive() && conflict->getResolution() == merge::ResolutionType::Unresolved;
    });
    
    if (activeConflict != _actions.end())
    {
        return merge::ActionType::ConflictResolution;
    }

    return !_actions.empty() ? merge::ActionType::ChangeKeyValue : merge::ActionType::NoAction;
}

std::size_t KeyValueMergeActionNode::getMergeActionCount()
{
    return _actions.size();
}

bool KeyValueMergeActionNode::hasActiveActions()
{
    for (const auto& action : _actions)
    {
        if (action->isActive()) return true;
    }

    return false;
}

void KeyValueMergeActionNode::foreachMergeAction(const std::function<void(const merge::IMergeAction::Ptr&)>& functor)
{
    for (const auto& action : _actions)
    {
        functor(action);
    }
}

// RegularMergeActionNode

RegularMergeActionNode::RegularMergeActionNode(const merge::IMergeAction::Ptr& action) :
    _action(action)
{
    _affectedNode = _action->getAffectedNode();
}

void RegularMergeActionNode::onInsertIntoScene(IMapRootNode& rootNode)
{
    // Add the nodes that are missing in this scene, for preview purposes
    addPreviewNodeForAddAction();

    // Let the base method hide the affected nodes
    MergeActionNodeBase::onInsertIntoScene(rootNode);
}

void RegularMergeActionNode::onRemoveFromScene(IMapRootNode& rootNode)
{
    MergeActionNodeBase::onRemoveFromScene(rootNode);

    removePreviewNodeForAddAction();
}

void RegularMergeActionNode::clear()
{
    _action.reset();
}

merge::ActionType RegularMergeActionNode::getActionType() const
{
    if (!_action) return merge::ActionType::NoAction;

    if (_action->getType() == merge::ActionType::ConflictResolution)
    {
        auto conflictAction = std::dynamic_pointer_cast<merge::IConflictResolutionAction>(_action);
        assert(conflictAction);

        // Determine how this node should be rendered (unresolved conflict, or the type of the change that was accepted)
        switch (conflictAction->getResolution())
        {
        case merge::ResolutionType::Unresolved:
            return merge::ActionType::ConflictResolution;

        case merge::ResolutionType::ApplySourceChange: // render using the accepted action type
            return conflictAction->getSourceAction()->getType();

        case merge::ResolutionType::RejectSourceChange:
            return merge::ActionType::NoAction;
        }
    }

    return _action->getType();
}

void RegularMergeActionNode::foreachMergeAction(const std::function<void(const merge::IMergeAction::Ptr&)>& functor)
{
    if (_action)
    {
        functor(_action);
    }
}

std::size_t RegularMergeActionNode::getMergeActionCount()
{
    return _action ? 1 : 0;
}

bool RegularMergeActionNode::hasActiveActions()
{
    return _action && _action->isActive();
}

std::shared_ptr<merge::AddCloneToParentAction> RegularMergeActionNode::getAddNodeAction()
{
    // In case this is a conflicting source action modified an entity that is no longer present, add the old node
    auto conflictAction = std::dynamic_pointer_cast<merge::IConflictResolutionAction>(_action);

    if (conflictAction && conflictAction->getConflictType() == merge::ConflictType::ModificationOfRemovedEntity)
    {
        return std::dynamic_pointer_cast<merge::AddCloneToParentAction>(conflictAction->getSourceAction());
    }

    // Check the regular action for type AddEntityNode
    return std::dynamic_pointer_cast<merge::AddCloneToParentAction>(_action);
}

void RegularMergeActionNode::addPreviewNodeForAddAction()
{
    auto addNodeAction = getAddNodeAction();

    if (addNodeAction)
    {
        // Get the clone and add it to the target scene, it needs to be renderable here
        addNodeAction->addSourceNodeToScene();
    }
}

void RegularMergeActionNode::removePreviewNodeForAddAction()
{
    auto addNodeAction = getAddNodeAction();

    if (addNodeAction)
    {
        addNodeAction->removeSourceNodeFromScene();
    }
}

}

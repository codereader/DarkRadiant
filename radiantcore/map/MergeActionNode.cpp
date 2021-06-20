#include "MergeActionNode.h"

namespace map
{

MergeActionNodeBase::MergeActionNodeBase() :
    _syncActionStatus(true)
{}

void MergeActionNodeBase::prepareForMerge()
{
    _syncActionStatus = false;
}

scene::INodePtr MergeActionNodeBase::getAffectedNode()
{
    return _affectedNode;
}

void MergeActionNodeBase::clear()
{
    _affectedNode.reset();
}

void MergeActionNodeBase::onInsertIntoScene(scene::IMapRootNode& rootNode)
{
    if (_syncActionStatus)
    {
        foreachMergeAction([&](const scene::merge::IMergeAction::Ptr& action)
        {
            action->activate();
        });
    }

    hideAffectedNodes();

    SelectableNode::onInsertIntoScene(rootNode);
}

void MergeActionNodeBase::onRemoveFromScene(scene::IMapRootNode& rootNode)
{
    SelectableNode::onRemoveFromScene(rootNode);
    
    unhideAffectedNodes();

    if (_syncActionStatus)
    {
        foreachMergeAction([&](const scene::merge::IMergeAction::Ptr& action)
        {
            // Removing an unresolved conflict action from the scene implies rejecting the source change
            auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);
            
            if (conflictAction && conflictAction->getResolution() == scene::merge::ResolutionType::Unresolved)
            {
                conflictAction->setResolution(scene::merge::ResolutionType::RejectSourceChange);
            }

            // Removing any action from the scene means to deactivate it
            action->deactivate();
        });
    }
}

scene::INode::Type MergeActionNodeBase::getNodeType() const
{
    return scene::INode::Type::MergeAction;
}

bool MergeActionNodeBase::supportsStateFlag(unsigned int state) const
{
    if ((state & (eHidden | eFiltered | eExcluded | eLayered)) != 0)
    {
        return false; // don't allow this node to be hidden
    }

    return Node::supportsStateFlag(state);
}

const AABB& MergeActionNodeBase::localAABB() const
{
    return _affectedNode->localAABB();
}

void MergeActionNodeBase::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
    _affectedNode->viewChanged();
    _affectedNode->renderSolid(collector, volume);
    _affectedNode->foreachNode([&](const scene::INodePtr& child)
    {
        child->viewChanged();
        child->renderSolid(collector, volume);
        return true;
    });
}

void MergeActionNodeBase::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
    _affectedNode->viewChanged();
    _affectedNode->renderWireframe(collector, volume);
    _affectedNode->foreachNode([&](const scene::INodePtr& child)
    {
        child->viewChanged();
        child->renderWireframe(collector, volume);
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

    _affectedNode->foreachNode([&](const scene::INodePtr& child)
    {
        testSelectNode(child, selector, test);
        return true;
    });
}

void MergeActionNodeBase::testSelectNode(const scene::INodePtr& node, Selector& selector, SelectionTest& test)
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

    _affectedNode->foreachNode([&](const scene::INodePtr& child)
    {
        child->enable(Node::eExcluded);
        return true;
    });
}

void MergeActionNodeBase::unhideAffectedNodes()
{
    // Release the excluded state of the contained nodes
    _affectedNode->disable(Node::eExcluded);

    _affectedNode->foreachNode([&](const scene::INodePtr& child)
    {
        child->disable(Node::eExcluded);
        return true;
    });
}

// ------------ KeyValueMergeActionNode ----------------------------

KeyValueMergeActionNode::KeyValueMergeActionNode(const std::vector<scene::merge::IMergeAction::Ptr>& actions) :
    _actions(actions)
{
    assert(!_actions.empty());

    _affectedNode = _actions.front()->getAffectedNode();
    assert(std::find_if(_actions.begin(), _actions.end(),
        [&](const scene::merge::IMergeAction::Ptr& action) { return action->getAffectedNode() != _affectedNode; }) == _actions.end());
}

void KeyValueMergeActionNode::clear()
{
    _actions.clear();
}

scene::merge::ActionType KeyValueMergeActionNode::getActionType() const
{
    // We report the change key value type since we're doing all kinds of key value changes,
    // unless we have an unresolved conflict in our collection
    auto activeConflict = std::find_if(_actions.begin(), _actions.end(), [&](const scene::merge::IMergeAction::Ptr& action) 
    {
        auto conflict = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

        return conflict && conflict->isActive() && conflict->getResolution() == scene::merge::ResolutionType::Unresolved;
    });
    
    if (activeConflict != _actions.end())
    {
        return scene::merge::ActionType::ConflictResolution;
    }

    return !_actions.empty() ? scene::merge::ActionType::ChangeKeyValue : scene::merge::ActionType::NoAction;
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

void KeyValueMergeActionNode::foreachMergeAction(const std::function<void(const scene::merge::IMergeAction::Ptr&)>& functor)
{
    for (const auto& action : _actions)
    {
        functor(action);
    }
}

// RegularMergeActionNode

RegularMergeActionNode::RegularMergeActionNode(const scene::merge::IMergeAction::Ptr& action) :
    _action(action)
{
    _affectedNode = _action->getAffectedNode();
}

void RegularMergeActionNode::onInsertIntoScene(scene::IMapRootNode& rootNode)
{
    // Add the nodes that are missing in this scene, for preview purposes
    addPreviewNodeForAddAction();

    // Let the base method hide the affected nodes
    MergeActionNodeBase::onInsertIntoScene(rootNode);
}

void RegularMergeActionNode::onRemoveFromScene(scene::IMapRootNode& rootNode)
{
    MergeActionNodeBase::onRemoveFromScene(rootNode);

    removePreviewNodeForAddAction();
}

void RegularMergeActionNode::clear()
{
    _action.reset();
}

scene::merge::ActionType RegularMergeActionNode::getActionType() const
{
    if (!_action) return scene::merge::ActionType::NoAction;

    if (_action->getType() == scene::merge::ActionType::ConflictResolution)
    {
        auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(_action);
        assert(conflictAction);

        // Determine how this node should be rendered (unresolved conflict, or the type of the change that was accepted)
        switch (conflictAction->getResolution())
        {
        case scene::merge::ResolutionType::Unresolved:
            return scene::merge::ActionType::ConflictResolution;

        case scene::merge::ResolutionType::ApplySourceChange: // render using the accepted action type
            return conflictAction->getSourceAction()->getType();

        case scene::merge::ResolutionType::RejectSourceChange:
            return scene::merge::ActionType::NoAction;
        }
    }

    return _action->getType();
}

void RegularMergeActionNode::foreachMergeAction(const std::function<void(const scene::merge::IMergeAction::Ptr&)>& functor)
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

std::shared_ptr<scene::merge::AddCloneToParentAction> RegularMergeActionNode::getAddNodeAction()
{
    // In case this is a conflicting source action modified an entity that is no longer present, add the old node
    auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(_action);

    if (conflictAction && conflictAction->getConflictType() == scene::merge::ConflictType::ModificationOfRemovedEntity)
    {
        return std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(conflictAction->getSourceAction());
    }

    // Check the regular action for type AddEntityNode
    return std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);
}

void RegularMergeActionNode::addPreviewNodeForAddAction()
{
    auto addNodeAction = getAddNodeAction();

    if (addNodeAction)
    {
        // Get the clone and add it to the target scene, it needs to be renderable here
        scene::addNodeToContainer(_affectedNode, addNodeAction->getParent());
    }
}

void RegularMergeActionNode::removePreviewNodeForAddAction()
{
    auto addNodeAction = getAddNodeAction();

    if (addNodeAction)
    {
        scene::removeNodeFromParent(_affectedNode);
    }
}

}

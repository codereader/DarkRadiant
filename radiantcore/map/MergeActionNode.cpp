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

void MergeActionNodeBase::onInsertIntoScene(scene::IMapRootNode& rootNode)
{
    SelectableNode::onInsertIntoScene(rootNode);

    hideAffectedNodes();
}

void MergeActionNodeBase::onRemoveFromScene(scene::IMapRootNode& rootNode)
{
    unhideAffectedNodes();

    SelectableNode::onRemoveFromScene(rootNode);
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

KeyValueMergeActionNode::KeyValueMergeActionNode(const std::vector<scene::merge::MergeAction::Ptr>& actions) :
    _actions(actions)
{
    assert(!_actions.empty());

    _affectedNode = _actions.front()->getAffectedNode();
    assert(std::find_if(_actions.begin(), _actions.end(),
        [&](const scene::merge::MergeAction::Ptr& action) { return action->getAffectedNode() != _affectedNode; }) == _actions.end());
}

scene::merge::ActionType KeyValueMergeActionNode::getActionType() const
{
    // We report the change key value type since we're doing all kinds of key value changes
    return scene::merge::ActionType::ChangeKeyValue;
}

void KeyValueMergeActionNode::onInsertIntoScene(scene::IMapRootNode& rootNode)
{
    if (_syncActionStatus)
    {
        for (const auto& action : _actions)
        {
            action->activate();
        }
    }

    MergeActionNodeBase::onInsertIntoScene(rootNode);
}

void KeyValueMergeActionNode::onRemoveFromScene(scene::IMapRootNode& rootNode)
{
    MergeActionNodeBase::onRemoveFromScene(rootNode);

    if (_syncActionStatus)
    {
        for (const auto& action : _actions)
        {
            action->deactivate();
        }
    }
}

// RegularMergeActionNode

RegularMergeActionNode::RegularMergeActionNode(const scene::merge::MergeAction::Ptr& action) :
    _action(action)
{
    _affectedNode = _action->getAffectedNode();
}

void RegularMergeActionNode::onInsertIntoScene(scene::IMapRootNode& rootNode)
{
    if (_syncActionStatus)
    {
        _action->activate();
    }

    // Add the nodes that are missing in this scene, for preview purposes
    addPreviewNodeForAddAction();

    // Let the base method hide the affected nodes
    MergeActionNodeBase::onInsertIntoScene(rootNode);
}

void RegularMergeActionNode::onRemoveFromScene(scene::IMapRootNode& rootNode)
{
    MergeActionNodeBase::onRemoveFromScene(rootNode);

    removePreviewNodeForAddAction();

    if (_syncActionStatus)
    {
        _action->deactivate();
    }
}

scene::merge::ActionType RegularMergeActionNode::getActionType() const
{
    return _action->getType();
}

void RegularMergeActionNode::addPreviewNodeForAddAction()
{
    // We add the node to the target scene, for preview purposes
    auto addNodeAction = std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);

    if (addNodeAction)
    {
        // Get the clone and add it to the target scene, it needs to be renderable here
        scene::addNodeToContainer(_affectedNode, addNodeAction->getParent());
    }
}

void RegularMergeActionNode::removePreviewNodeForAddAction()
{
    auto addNodeAction = std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);

    if (addNodeAction)
    {
        scene::removeNodeFromParent(_affectedNode);
    }
}

}

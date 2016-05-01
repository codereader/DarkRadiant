#include "TargetableNode.h"

#include "TargetManager.h"
#include "../EntityNode.h"
#include "TargetLineNode.h"

namespace entity {

TargetableNode::TargetableNode(Doom3Entity& entity, EntityNode& node, const ShaderPtr& wireShader) :
	_d3entity(entity),
    _targetKeys(*this),
	_renderableLines(_targetKeys),
	_node(node),
	_wireShader(wireShader),
    _targetManager(nullptr)
{
	// Note: don't do anything with _d3Entity here,
	// the structure is not fully constructed yet at this point.
	// Execute initialisation code in construct()
}

ITargetManager* TargetableNode::getTargetManager()
{
    return _targetManager;
}

void TargetableNode::construct()
{
	_d3entity.attachObserver(this);
	_d3entity.attachObserver(&_targetKeys);
}

// Disconnect this class from the entity
void TargetableNode::destruct()
{
	_d3entity.detachObserver(&_targetKeys);
	_d3entity.detachObserver(this);
}

const TargetKeyCollection& TargetableNode::getTargetKeys() const
{
    return _targetKeys;
}

// Gets called as soon as the "name" keyvalue changes
void TargetableNode::onKeyValueChanged(const std::string& name)
{
	// Check if we were registered before
    if (!_targetName.empty() && _targetManager)
    {
		// Old name is not empty
		// Tell the Manager to disassociate us from the target
        _targetManager->clearTarget(_targetName, _node);
	}

	// Store the new name, in any case
	_targetName = name;

	if (_targetName.empty())
    {
		// New name is empty, do not associate
		return;
	}

	// Tell the TargetManager to associate the name with this scene::INode here
    if (_targetManager)
    {
        _targetManager->associateTarget(_targetName, _node);
    }
}

// Entity::Observer implementation, gets called on key insert
void TargetableNode::onKeyInsert(const std::string& key, EntityKeyValue& value) 
{
	if (key == "name")
	{
		// Subscribe to this keyvalue to get notified about "name" changes
		value.attach(*this);
	}
}

// Entity::Observer implementation, gets called on key erase
void TargetableNode::onKeyErase(const std::string& key, EntityKeyValue& value)
{
	if (key == "name")
    {
		// Unsubscribe from this keyvalue
		value.detach(*this);
	}
}

void TargetableNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    _targetManager = &root.getTargetManager();

    // Now that we're in the scene, register this name if we have one already
    if (!_targetName.empty() && _targetManager)
    {
        _targetManager->associateTarget(_targetName, _node);
    }

    // Notify the underlying key collection to reacquire their targets
    _targetKeys.onTargetManagerChanged();
}

void TargetableNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    // On scene removal, unregister this name if we have one
    if (!_targetName.empty() && _targetManager)
    {
        _targetManager->clearTarget(_targetName, _node);
    }

    _targetManager = nullptr;

    // Notify the underlying key collection to clear their target references
    _targetKeys.onTargetManagerChanged();
}

const Vector3& TargetableNode::getWorldPosition() const
{
	const AABB& bounds = _node.worldAABB();

	if (bounds.isValid())
    {
		return bounds.getOrigin();
	}

	return _node.localToWorld().t().getVector3();
}

void TargetableNode::render(RenderableCollector& collector, const VolumeTest& volume) const
{
	//if (!_renderableLines.hasTargets() || !_node.visible()) return;
    //
	//collector.SetState(_wireShader, RenderableCollector::eWireframeOnly);
	//collector.SetState(_wireShader, RenderableCollector::eFullMaterials);
	//_renderableLines.render(collector, volume, getWorldPosition());
}

void TargetableNode::onTargetKeyCollectionChanged()
{
    if (!_targetKeys.empty())
    {
        // Add TargetLineNode as child
        if (!_targetLineNode)
        {
            _targetLineNode.reset(new TargetLineNode(_node));
            scene::addNodeToContainer(_targetLineNode, _node.shared_from_this());
        }
    }
    else // No more targets
    {
        // Clear child TargetLineNode
        if (_targetLineNode)
        {
            scene::removeNodeFromParent(_targetLineNode);
            _targetLineNode.reset();
        }
    }
}

} // namespace entity

#include "TargetKeyCollection.h"

#include "TargetableNode.h"
#include "string/predicate.h"

namespace entity {

TargetKeyCollection::TargetKeyCollection(TargetableNode& owner) :
    _owner(owner)
{}

ITargetManager* TargetKeyCollection::getTargetManager()
{
    return _owner.getTargetManager();
}

void TargetKeyCollection::onTargetManagerChanged()
{
    for (auto& pair : _targetKeys)
    {
        pair.second.onTargetManagerChanged();
    }
}

void TargetKeyCollection::forEachTarget(const std::function<void(const TargetPtr&)>& func) const
{
	for (auto pair : _targetKeys)
	{
		func(pair.second.getTarget());
	}
}

std::size_t TargetKeyCollection::getNumTargets() const
{
    return _targetKeys.size();
}

bool TargetKeyCollection::empty() const
{
	return _targetKeys.empty();
}

bool TargetKeyCollection::isTargetKey(const std::string& key) 
{
	// A key is a target key if it starts with "target" (any case)
	return (string::istarts_with(key, "target"));
}

// Entity::Observer implementation, gets called on key insert
void TargetKeyCollection::onKeyInsert(const std::string& key, EntityKeyValue& value)
{
	// ignore non-target keys
	if (!isTargetKey(key))
    {
		return;
	}

	TargetKeyMap::iterator i = _targetKeys.insert(std::make_pair(key, TargetKey(*this))).first;

	i->second.attachToKeyValue(value);

    // Notify the owning node to create the TargetLineNode
    _owner.onTargetKeyCollectionChanged();
}

// Entity::Observer implementation, gets called on key erase
void TargetKeyCollection::onKeyErase(const std::string& key, EntityKeyValue& value)
{
	// ignore non-target keys
	if (!isTargetKey(key))
    {
		return;
	}

	TargetKeyMap::iterator i = _targetKeys.find(key);

	// This must be found
	assert(i != _targetKeys.end());

	i->second.detachFromKeyValue(value);

	// Remove the found element
	_targetKeys.erase(i);

    // Notify the owner to destruct the target line node
    _owner.onTargetKeyCollectionChanged();
}

} // namespace entity

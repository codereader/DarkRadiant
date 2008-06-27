#include "EntityNode.h"

namespace entity {

EntityNode::EntityNode(IEntityClassPtr eclass) :
	_eclass(eclass),
	_entity(_eclass),
	_namespaceManager(_entity)
{}

EntityNode::EntityNode(const EntityNode& other) :
	_eclass(other._eclass),
	_entity(other._entity),
	_namespaceManager(_entity)
{}

std::string EntityNode::getName() {
	return _namespaceManager.getName();
}

void EntityNode::setNamespace(INamespace* space) {
	_namespaceManager.setNamespace(space);
}

INamespace* EntityNode::getNamespace() const {
	return _namespaceManager.getNamespace();
}

void EntityNode::connectNameObservers() {
	_namespaceManager.connectNameObservers();
}

void EntityNode::disconnectNameObservers() {
	_namespaceManager.disconnectNameObservers();
}

void EntityNode::attachNames() {
	_namespaceManager.attachNames();
}

void EntityNode::detachNames() {
	_namespaceManager.detachNames();
}

void EntityNode::changeName(const std::string& newName) {
	_namespaceManager.changeName(newName);
}

} // namespace entity

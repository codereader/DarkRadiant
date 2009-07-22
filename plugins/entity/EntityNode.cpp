#include "EntityNode.h"

#include "EntitySettings.h"

namespace entity {

EntityNode::EntityNode(const IEntityClassConstPtr& eclass) :
	TargetableNode(_entity, *this),
	_eclass(eclass),
	_entity(_eclass),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey)
{
	construct();
}

EntityNode::EntityNode(const EntityNode& other) :
	IEntityNode(other),
	SelectableNode(other),
	Namespaced(other),
	TargetableNode(_entity, *this),
	Renderable(other),
	Nameable(other),
	_eclass(other._eclass),
	_entity(other._entity),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey)
{
	construct();
}

EntityNode::~EntityNode()
{
	destruct();
}

void EntityNode::construct()
{
	TargetableNode::construct();
}

void EntityNode::destruct()
{
	TargetableNode::destruct();
}

std::string EntityNode::getName() const {
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

std::string EntityNode::name() const
{
	return _nameKey.name();
}

void EntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Nothing so far (FIXME)
}

void EntityNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Submit renderable text name if required
	if (collector.getStyle() == RenderableCollector::eWireframeOnly && 
		EntitySettings::InstancePtr()->renderEntityNames()) 
    {
		collector.SetState(_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
		collector.addRenderable(_renderableName, _renderableName.getLocalToParent());
	}
}

void EntityNode::_onTransformationChanged()
{
	_renderableName.revertTransform();
	_renderableName.translate(getTranslation());
}

void EntityNode::_applyTransformation()
{
	_renderableName.revertTransform();
	_renderableName.translate(getTranslation());
	_renderableName.freezeTransform();
}

} // namespace entity

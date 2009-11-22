#include "EntityNode.h"

#include "EntitySettings.h"

namespace entity {

EntityNode::EntityNode(const IEntityClassPtr& eclass) :
	TargetableNode(_entity, *this),
	_eclass(eclass),
	_entity(_eclass),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_keyObservers(_entity)
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
	Transformable(other),
	MatrixTransform(other),
	scene::Cloneable(other),
	_eclass(other._eclass),
	_entity(other._entity),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_keyObservers(_entity)
{
	construct();
}

EntityNode::~EntityNode()
{
	destruct();
}

void EntityNode::construct()
{
	_eclass->addObserver(this);

	TargetableNode::construct();

	addKeyObserver("name", NameKey::NameChangedCaller(_nameKey));
}

void EntityNode::destruct()
{
	removeKeyObserver("name", NameKey::NameChangedCaller(_nameKey));

	TargetableNode::destruct();

	_eclass->removeObserver(this);
}

void EntityNode::addKeyObserver(const std::string& key, const KeyObserver& observer)
{
	_keyObservers.insert(key, observer);
}

void EntityNode::removeKeyObserver(const std::string& key, const KeyObserver& observer)
{
	_keyObservers.erase(key, observer);
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

void EntityNode::instantiate(const scene::Path& path)
{
	_entity.instanceAttach(scene::findMapFile(path.top()));

	Node::instantiate(path);
}

void EntityNode::uninstantiate(const scene::Path& path)
{
	Node::uninstantiate(path);

	_entity.instanceDetach(scene::findMapFile(path.top()));
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
		collector.addRenderable(_renderableName, localToWorld());
	}
}

void EntityNode::OnEClassReload()
{
	// Let the keyobservers reload their values
	_keyObservers.refreshObservers();
}

} // namespace entity

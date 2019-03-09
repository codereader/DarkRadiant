#include "EntityNode.h"

#include "i18n.h"
#include "itextstream.h"
#include "icounter.h"

#include "EntitySettings.h"

namespace entity
{

EntityNode::EntityNode(const IEntityClassPtr& eclass) :
	TargetableNode(_entity, *this),
	_eclass(eclass),
	_entity(_eclass),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_modelKey(*this),
	_keyObservers(_entity),
	_shaderParms(_keyObservers, _colourKey),
	_direction(1,0,0)
{}

EntityNode::EntityNode(const EntityNode& other) :
	IEntityNode(other),
	SelectableNode(other),
	SelectionTestable(other),
	Namespaced(other),
	TargetableNode(_entity, *this),
	Transformable(other),
	MatrixTransform(other),
	scene::Cloneable(other),
	_eclass(other._eclass),
	_entity(other._entity),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_modelKey(*this),
	_keyObservers(_entity),
	_shaderParms(_keyObservers, _colourKey),
	_direction(1,0,0)
{}

EntityNode::~EntityNode()
{
	destruct();
}

void EntityNode::construct()
{
	_eclassChangedConn = _eclass->changedSignal().connect([this]()
	{
		this->onEntityClassChanged();
	});

	TargetableNode::construct();

	addKeyObserver("name", _nameKey);
	addKeyObserver("_color", _colourKey);

	_modelKeyObserver.setCallback(std::bind(&EntityNode::_modelKeyChanged, this, std::placeholders::_1));
	addKeyObserver("model", _modelKeyObserver);

	// Connect the skin keyvalue change handler directly to the model node manager
	_skinKeyObserver.setCallback(std::bind(&ModelKey::skinChanged, &_modelKey, std::placeholders::_1));
	addKeyObserver("skin", _skinKeyObserver);

	_shaderParms.addKeyObservers();
}

void EntityNode::destruct()
{
	_shaderParms.removeKeyObservers();

	removeKeyObserver("skin", _skinKeyObserver);

	_modelKey.setActive(false); // disable callbacks during destruction
	removeKeyObserver("model", _modelKeyObserver);

	removeKeyObserver("_color", _colourKey);
	removeKeyObserver("name", _nameKey);

	_eclassChangedConn.disconnect();

	TargetableNode::destruct();
}

void EntityNode::onEntityClassChanged()
{
	// By default, we notify the KeyObservers attached to this entity
	_keyObservers.refreshObservers();
}

void EntityNode::addKeyObserver(const std::string& key, KeyObserver& observer)
{
	_keyObservers.insert(key, observer);
}

void EntityNode::removeKeyObserver(const std::string& key, KeyObserver& observer)
{
	_keyObservers.erase(key, observer);
}

Entity& EntityNode::getEntity()
{
	return _entity;
}

void EntityNode::refreshModel()
{
	_modelKey.refreshModel();
}

float EntityNode::getShaderParm(int parmNum) const
{
	return _shaderParms.getParmValue(parmNum);
}

const Vector3& EntityNode::getDirection() const
{
	return _direction;
}

void EntityNode::testSelect(Selector& selector, SelectionTest& test)
{
	test.BeginMesh(localToWorld());

	// Pass the call down to the model node, if applicable
	SelectionTestablePtr selectionTestable = Node_getSelectionTestable(_modelKey.getNode());

    if (selectionTestable)
	{
		selectionTestable->testSelect(selector, test);
    }
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

void EntityNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    GlobalCounters().getCounter(counterEntities).increment();

	_entity.connectUndoSystem(root.getUndoChangeTracker());
	_modelKey.connectUndoSystem(root.getUndoChangeTracker());

	SelectableNode::onInsertIntoScene(root);
    TargetableNode::onInsertIntoScene(root);
}

void EntityNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    TargetableNode::onRemoveFromScene(root);
	SelectableNode::onRemoveFromScene(root);

	_modelKey.disconnectUndoSystem(root.getUndoChangeTracker());
	_entity.disconnectUndoSystem(root.getUndoChangeTracker());

    GlobalCounters().getCounter(counterEntities).decrement();
}

void EntityNode::onChildAdded(const scene::INodePtr& child)
{
	// Let the child know which renderEntity it has - this has to happen before onChildAdded()
	child->setRenderEntity(this);

	Node::onChildAdded(child);
}

void EntityNode::onChildRemoved(const scene::INodePtr& child)
{
	Node::onChildRemoved(child);

	// Leave the renderEntity on the child until this point - this has to happen after onChildRemoved()

	// greebo: Double-check that we're the currently assigned renderentity - in some cases nodes on the undostack
	// keep references to child nodes - we should never NULLify renderentities of nodes that are not assigned to us
	IRenderEntity* curRenderEntity = child->getRenderEntity();

	if (curRenderEntity && curRenderEntity == this)
	{
		child->setRenderEntity(nullptr);
	}
	else
	{
		rWarning() << "[EntityNode] the child being removed is already assigned to a different render entity." << std::endl;
	}
}

std::string EntityNode::name() const
{
	return _nameKey.name();
}

scene::INode::Type EntityNode::getNodeType() const
{
	return Type::Entity;
}

void EntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
    // Nothing here
}

void EntityNode::renderWireframe(RenderableCollector& collector,
                                 const VolumeTest& volume) const
{
	// Submit renderable text name if required
	if (EntitySettings::InstancePtr()->renderEntityNames())
    {
		collector.addRenderable(getWireShader(), _renderableName, localToWorld());
	}
}

void EntityNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	if (renderSystem)
	{
		_fillShader = renderSystem->capture(_entity.getEntityClass()->getFillShader());
		_wireShader = renderSystem->capture(_entity.getEntityClass()->getWireShader());
	}
	else
	{
		_fillShader.reset();
		_wireShader.reset();
	}

	// The colour key is maintaining a shader object as well
	_colourKey.setRenderSystem(renderSystem);
}

std::size_t EntityNode::getHighlightFlags()
{
	if (!isSelected()) return Highlight::NoHighlight;

	return isGroupMember() ? (Highlight::Selected | Highlight::GroupMember) : Highlight::Selected;
}

const Vector3& EntityNode::getColour() const
{
	return _colourKey.getColour();
}

const ShaderPtr& EntityNode::getColourShader() const
{
	return _colourKey.getWireShader();
}

ModelKey& EntityNode::getModelKey()
{
	return _modelKey;
}

void EntityNode::onModelKeyChanged(const std::string& value)
{
	// Default implementation suitable for Light, Generic and EClassModel
	// Dispatch the call to the key observer, which will create the child node
	_modelKey.modelChanged(value);
}

void EntityNode::_modelKeyChanged(const std::string& value)
{
	// Wrap the call to the virtual event
	onModelKeyChanged(value);
}

const ShaderPtr& EntityNode::getWireShader() const
{
	return _wireShader;
}

const ShaderPtr& EntityNode::getFillShader() const
{
	return _fillShader;
}

void EntityNode::onPostUndo()
{
	// After undo operations there might remain some child nodes
	// without renderentity, rectify that
	foreachNode([&] (const scene::INodePtr& child)->bool
	{
		child->setRenderEntity(this);
		return true;
	});
}

void EntityNode::onPostRedo()
{
	// After redo operations there might remain some child nodes
	// without renderentity, rectify that
	foreachNode([&] (const scene::INodePtr& child)->bool
	{
		child->setRenderEntity(this);
		return true;
	});
}

} // namespace entity

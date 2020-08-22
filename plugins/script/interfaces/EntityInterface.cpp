#include "EntityInterface.h"

#include <pybind11/stl_bind.h>

#include "ientity.h"
#include "ieclass.h"
#include "itextstream.h"

#include "../SceneNodeBuffer.h"

namespace script 
{

// Constructor, checks if the passed node is actually an entity
ScriptEntityNode::ScriptEntityNode(const scene::INodePtr& node) :
	ScriptSceneNode((node != NULL && Node_isEntity(node)) ? node : scene::INodePtr())
{}

// Methods wrapping to Entity class
std::string ScriptEntityNode::getKeyValue(const std::string& key) {
	Entity* entity = Node_getEntity(*this);
	return (entity != NULL) ? entity->getKeyValue(key) : "";
}

void ScriptEntityNode::setKeyValue(const std::string& key, const std::string& value) {
	Entity* entity = Node_getEntity(*this);

	if (entity != NULL) {
		entity->setKeyValue(key, value);
	}
}

bool ScriptEntityNode::isInherited(const std::string& key) {
	Entity* entity = Node_getEntity(*this);

	return (entity != NULL) ? entity->isInherited(key) : false;
}

ScriptEntityClass ScriptEntityNode::getEntityClass() {
	Entity* entity = Node_getEntity(*this);
	return ScriptEntityClass(entity != NULL ? entity->getEntityClass() : IEntityClassPtr());
}

bool ScriptEntityNode::isModel() {
	Entity* entity = Node_getEntity(*this);
	return (entity != NULL) ? entity->isModel() : false;
}

bool ScriptEntityNode::isOfType(const std::string& className)
{
	Entity* entity = Node_getEntity(*this);
	return entity == NULL ? false : entity->isOfType(className);
}

Entity::KeyValuePairs ScriptEntityNode::getKeyValuePairs(const std::string& prefix) {
	Entity* entity = Node_getEntity(*this);
	return (entity != NULL) ? entity->getKeyValuePairs(prefix) : Entity::KeyValuePairs();
}

void ScriptEntityNode::forEachKeyValue(EntityVisitor& visitor) {
	Entity* entity = Node_getEntity(*this);

	if (entity)
    {
        entity->forEachKeyValue([&](const std::string& key, const std::string& value)
        {
            visitor.visit(key, value);
        });
	}
}

// Checks if the given SceneNode structure is a BrushNode
bool ScriptEntityNode::isEntity(const ScriptSceneNode& node) {
	return Node_isEntity(node);
}

// "Cast" service for Python, returns a ScriptEntityNode.
// The returned node is non-NULL if the cast succeeded
ScriptEntityNode ScriptEntityNode::getEntity(const ScriptSceneNode& node) {
	// Try to cast the node onto a brush
	IEntityNodePtr entityNode = std::dynamic_pointer_cast<IEntityNode>(
		static_cast<scene::INodePtr>(node)
	);

	// Construct a entityNode (contained node is NULL if not an entity)
	return ScriptEntityNode(entityNode != NULL
                           ? node
                           : ScriptSceneNode(scene::INodePtr()));
}

// Creates a new entity for the given entityclass
ScriptSceneNode EntityInterface::createEntity(const ScriptEntityClass& eclass)
{
	scene::INodePtr node = GlobalEntityModule().createEntity(eclass);

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

// Creates a new entity for the given entityclass
ScriptSceneNode EntityInterface::createEntity(const std::string& eclassName) {
	// Find the eclass
	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(eclassName);

	if (eclass == NULL) {
		rMessage() << "Could not find entity class: " << eclassName << std::endl;
		return ScriptSceneNode(scene::INodePtr());
	}

	scene::INodePtr node = GlobalEntityModule().createEntity(eclass);

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

struct EntityKeyValuePair :
	public std::pair<std::string, std::string>
{
	using std::pair<std::string, std::string>::pair;

	std::string left()
	{
		return first;
	}

	std::string right()
	{
		return second;
	}
};

void EntityInterface::registerInterface(py::module& scope, py::dict& globals) 
{
	// Add the EntityNode interface
	py::class_<ScriptEntityNode, ScriptSceneNode> entityNode(scope, "EntityNode");
	entityNode.def(py::init<const scene::INodePtr&>());
	entityNode.def("getKeyValue", &ScriptEntityNode::getKeyValue);
	entityNode.def("setKeyValue", &ScriptEntityNode::setKeyValue);
	entityNode.def("forEachKeyValue", &ScriptEntityNode::forEachKeyValue);
	entityNode.def("isInherited", &ScriptEntityNode::isInherited);
	entityNode.def("getEntityClass", &ScriptEntityNode::getEntityClass);
	entityNode.def("isModel", &ScriptEntityNode::isModel);
	entityNode.def("isOfType", &ScriptEntityNode::isOfType);
	entityNode.def("getKeyValuePairs", &ScriptEntityNode::getKeyValuePairs);

	// Declare the KeyValuePairs vector
	py::bind_vector<Entity::KeyValuePairs>(scope, "EntityKeyValuePairs");

	// Expose the Entity::Visitor interface
	py::class_<EntityVisitor, EntityVisitorWrapper> visitor(scope, "EntityVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &EntityVisitor::visit);

	// Add the EntityCreator module declaration to the given python namespace
	py::class_<EntityInterface> entityCreator(scope, "EntityCreator");

	// Add both overloads to createEntity
	entityCreator.def("createEntity", static_cast<ScriptSceneNode(EntityInterface::*)(const std::string&)>(&EntityInterface::createEntity));
	entityCreator.def("createEntity", static_cast<ScriptSceneNode(EntityInterface::*)(const ScriptEntityClass&)>(&EntityInterface::createEntity));

	// Now point the Python variable "GlobalEntityCreator" to this instance
	globals["GlobalEntityCreator"] = this;
}

} // namespace script

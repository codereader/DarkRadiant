#include "EntityInterface.h"

#include "ientity.h"
#include "ieclass.h"
#include "itextstream.h"

#include "../SceneNodeBuffer.h"
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace script {

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

void ScriptEntityNode::forEachKeyValue(Entity::Visitor& visitor) {
	Entity* entity = Node_getEntity(*this);

	if (entity != NULL) {
		entity->forEachKeyValue(visitor);
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
	IEntityNodePtr entityNode = boost::dynamic_pointer_cast<IEntityNode>(
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
	scene::INodePtr node = GlobalEntityCreator().createEntity(eclass);

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

	scene::INodePtr node = GlobalEntityCreator().createEntity(eclass);

	// Add the node to the buffer otherwise it will be deleted immediately,
	// as ScriptSceneNodes are using weak_ptrs.
	SceneNodeBuffer::Instance().push_back(node);

	return ScriptSceneNode(node);
}

void EntityInterface::registerInterface(boost::python::object& nspace) {
	// Add the EntityNode interface
	nspace["EntityNode"] = boost::python::class_<ScriptEntityNode,
		boost::python::bases<ScriptSceneNode> >("EntityNode", boost::python::init<const scene::INodePtr&>() )
		.def("getKeyValue", &ScriptEntityNode::getKeyValue)
		.def("setKeyValue", &ScriptEntityNode::setKeyValue)
		.def("forEachKeyValue", &ScriptEntityNode::forEachKeyValue)
		.def("isInherited", &ScriptEntityNode::isInherited)
		.def("getEntityClass", &ScriptEntityNode::getEntityClass)
		.def("isModel", &ScriptEntityNode::isModel)
		.def("isOfType", &ScriptEntityNode::isOfType)
		.def("getKeyValuePairs", &ScriptEntityNode::getKeyValuePairs)
	;

	// Declare a KeyValuePair (pair of strings)
	boost::python::class_< std::pair<std::string, std::string> >(
		"EntityKeyValuePair", boost::python::init<std::string, std::string>())
		.def_readwrite("first", &std::pair<std::string, std::string>::first)
		.def_readwrite("second", &std::pair<std::string, std::string>::second)
	;

	// Declare the KeyValuePairs vector
	boost::python::class_<Entity::KeyValuePairs>("EntityKeyValuePairs")
		.def(boost::python::vector_indexing_suite<Entity::KeyValuePairs, true>())
	;

	// Add the "isEntity" and "getEntity" method to all ScriptSceneNodes
	boost::python::object sceneNode = nspace["SceneNode"];

	boost::python::objects::add_to_namespace(sceneNode,
		"isEntity", boost::python::make_function(&ScriptEntityNode::isEntity));

	boost::python::objects::add_to_namespace(sceneNode,
		"getEntity", boost::python::make_function(&ScriptEntityNode::getEntity));

	// Expose the Entity::Visitor interface
	nspace["EntityVisitor"] =
		boost::python::class_<EntityVisitorWrapper, boost::noncopyable>("EntityVisitor")
		.def("visit", boost::python::pure_virtual(&EntityVisitorWrapper::visit))
	;

	// Add the EntityCreator module declaration to the given python namespace
	nspace["GlobalEntityCreator"] = boost::python::class_<EntityInterface>("GlobalEntityCreator")
		// Add both overloads to createEntity
		.def("createEntity", static_cast<ScriptSceneNode (EntityInterface::*)(const std::string&)>(&EntityInterface::createEntity))
		.def("createEntity", static_cast<ScriptSceneNode (EntityInterface::*)(const ScriptEntityClass&)>(&EntityInterface::createEntity))
	;

	// Now point the Python variable "GlobalEntityCreator" to this instance
	nspace["GlobalEntityCreator"] = boost::python::ptr(this);
}

} // namespace script

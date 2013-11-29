#ifndef _ENTITY_INTERFACE_H_
#define _ENTITY_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "ientity.h"

#include "EClassInterface.h"
#include "SceneGraphInterface.h"

namespace script {

class ScriptEntityNode :
	public ScriptSceneNode
{
public:
	// Constructor, checks if the passed node is actually an entity
	ScriptEntityNode(const scene::INodePtr& node);

	// Methods wrapping to Entity class
	std::string getKeyValue(const std::string& key);
	void setKeyValue(const std::string& key, const std::string& value);
	bool isInherited(const std::string& key);

	ScriptEntityClass getEntityClass();
	bool isModel();
	bool isOfType(const std::string& className);
	Entity::KeyValuePairs getKeyValuePairs(const std::string& prefix);

	// Visit each keyvalue, wraps to the contained entity
	void forEachKeyValue(Entity::Visitor& visitor);

	// Checks if the given SceneNode structure is an EntityNode
	static bool isEntity(const ScriptSceneNode& node);

	// "Cast" service for Python, returns a ScriptEntityNode.
	// The returned node is non-NULL if the cast succeeded
	static ScriptEntityNode getEntity(const ScriptSceneNode& node);
};

// Wrap around the EntityClassVisitor interface
class EntityVisitorWrapper :
	public Entity::Visitor,
	public boost::python::wrapper<Entity::Visitor>
{
public:
	void visit(const std::string& key, const std::string& value) {
		// Wrap this method to python
		this->get_override("visit")(key, value);
	}
};

/**
 * greebo: This class registers the entity interface with the
 * scripting system.
 */
class EntityInterface :
	public IScriptInterface
{
public:
	// Creates a new entity for the given entityclass
	ScriptSceneNode createEntity(const ScriptEntityClass& eclass);

	// Creates a new entity for the named entityclass
	ScriptSceneNode createEntity(const std::string& eclassName);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<EntityInterface> EntityInterfacePtr;

} // namespace script

#endif /* _ENTITY_INTERFACE_H_ */

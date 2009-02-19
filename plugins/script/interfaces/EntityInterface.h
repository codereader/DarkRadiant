#ifndef _ENTITY_INTERFACE_H_
#define _ENTITY_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "ientity.h"

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

	// Checks if the given SceneNode structure is a BrushNode
	static bool isEntity(const ScriptSceneNode& node);

	// "Cast" service for Python, returns a ScriptEntityNode. 
	// The returned node is non-NULL if the cast succeeded
	static ScriptEntityNode getEntity(const ScriptSceneNode& node);
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
	ScriptSceneNode createEntity(const IEntityClassPtr& eclass);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<EntityInterface> EntityInterfacePtr;

} // namespace script

#endif /* _ENTITY_INTERFACE_H_ */

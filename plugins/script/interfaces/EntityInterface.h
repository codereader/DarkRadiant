#ifndef _ENTITY_INTERFACE_H_
#define _ENTITY_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "ientity.h"

#include "SceneGraphInterface.h"

namespace script {

class ScriptEntity;

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

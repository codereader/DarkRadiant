#include "EntityInterface.h"

#include "ientity.h"
#include "ieclass.h"

namespace script {

// Creates a new entity for the given entityclass
ScriptSceneNode EntityInterface::createEntity(const IEntityClassPtr& eclass) {
	return ScriptSceneNode(scene::INodePtr());
}

void EntityInterface::registerInterface(boost::python::object& nspace) {
	// Add the EntityCreator module declaration to the given python namespace
	nspace["GlobalEntityCreator"] = boost::python::class_<EntityInterface>("GlobalEntityCreator")
		.def("createEntity", &EntityInterface::createEntity)
	;

	// Now point the Python variable "GlobalEntityCreator" to this instance
	nspace["GlobalEntityCreator"] = boost::python::ptr(this);
}

} // namespace script

#include "RadiantInterface.h"

#include "iscenegraph.h"
#include "entitylib.h"

namespace script {

// A wrapper class representing the Entity class
class ScriptEntity
{
	// The contained entity object
	Entity* _entity;
public:
	ScriptEntity(Entity* entity) :
		_entity(entity)
	{}

	// Methods wrapping to Entity class
	std::string getKeyValue(const std::string& key) {
		return (_entity != NULL) ? _entity->getKeyValue(key) : "";
	}

	void setKeyValue(const std::string& key, const std::string& value) {
		if (_entity != NULL) {
			_entity->setKeyValue(key, value);
		}
	}
};

ScriptEntity* RadiantInterface::findEntityByClassname(const std::string& name) {
	EntityFindByClassnameWalker walker(name);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

	// Note: manage_new_object return value policy will take care of that raw pointer
	return new ScriptEntity(walker.getEntity());
}

void RadiantInterface::registerInterface(boost::python::object& nspace) {
	// Add the Entity class declaration
	nspace["Entity"] = boost::python::class_<ScriptEntity>("Entity", boost::python::init<Entity*>())
		.def("getKeyValue", &ScriptEntity::getKeyValue)
		.def("setKeyValue", &ScriptEntity::setKeyValue)
	;

	nspace["Radiant"] = boost::python::class_<RadiantInterface>("Radiant")
		.def("findEntityByClassname", &RadiantInterface::findEntityByClassname, 
				boost::python::return_value_policy<
					boost::python::manage_new_object, // transfer ownership of returned pointer
					boost::python::default_call_policies>())
	;

	// Point the radiant variable to this
	nspace["Radiant"] = boost::python::ptr(this);
}

} // namespace script

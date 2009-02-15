#ifndef _ECLASS_INTERFACE_H_
#define _ECLASS_INTERFACE_H_

#include "ieclass.h"
#include "iscript.h"
#include <boost/python.hpp>

namespace script {

/**
 * This class represents a single EntityDef / EntityClass for script.
 */
class ScriptEntityClass
{
	const IEntityClassConstPtr _eclass;
public:
	ScriptEntityClass(const IEntityClassConstPtr& eclass) :
		_eclass(eclass)
	{}

	// Returns a specific spawnarg from this entityDef, or "" if not found
	EntityClassAttribute getAttribute(const std::string& name) {
		return (_eclass != NULL) ? _eclass->getAttribute(name) : EntityClassAttribute();
	}
};

/**
 * greebo: This class provides the script interface for the GlobalEntityClassManager module.
 */
class EClassManagerInterface :
	public IScriptInterface
{
public:
	ScriptEntityClass findClass(const std::string& name) {
		// Find the eclass and convert implicitly to ScriptEntityClass
		return GlobalEntityClassManager().findClass(name);
	}

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace) {
		// Add the declaration for the EClassAttribute
		nspace["EntityClassAttribute"] = boost::python::class_<EntityClassAttribute>("EntityClassAttribute")
			.def_readwrite("type", &EntityClassAttribute::type)
			.def_readwrite("name", &EntityClassAttribute::name)
			.def_readwrite("value", &EntityClassAttribute::value)
			.def_readwrite("description", &EntityClassAttribute::description)
			.def_readwrite("inherited", &EntityClassAttribute::inherited)
		;

		// Add the declaration for an EntityClass
		nspace["EntityClass"] = boost::python::class_<ScriptEntityClass>(
			"EntityClass", boost::python::init<const IEntityClassConstPtr&>())
			.def("getAttribute", &ScriptEntityClass::getAttribute)
		;

		// Add the module declaration to the given python namespace
		nspace["GlobalEntityClassManager"] = boost::python::class_<EClassManagerInterface>("GlobalEntityClassManager")
			.def("findClass", &EClassManagerInterface::findClass)
		;

		// Now point the Python variable "GlobalRegistry" to this instance
		nspace["GlobalEntityClassManager"] = boost::python::ptr(this);
	}
};
typedef boost::shared_ptr<EClassManagerInterface> EClassManagerInterfacePtr;

} // namespace script

#endif /* _ECLASS_INTERFACE_H_ */

#ifndef _ECLASS_INTERFACE_H_
#define _ECLASS_INTERFACE_H_

#include "ieclass.h"
#include "iscript.h"
#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

namespace script {

/**
 * This class represents a single EntityDef / EntityClass for script.
 */
class ScriptEntityClass
{
	IEntityClassPtr _eclass;
	EntityClassAttribute _emptyAttribute;

public:
	ScriptEntityClass(const IEntityClassPtr& eclass) :
		_eclass(eclass)
	{}

	// Returns a specific spawnarg from this entityDef, or "" if not found
	EntityClassAttribute& getAttribute(const std::string& name) {
		if (_eclass == NULL) {
			return _emptyAttribute;
		}

		return _eclass->getAttribute(name);
	}
};

// Wrap around the EntityClassVisitor interface
class EntityClassVisitorWrapper : 
	public EntityClassVisitor, 
	public boost::python::wrapper<EntityClassVisitor>
{
public:
    void visit(IEntityClassPtr eclass) {
		// Wrap this method to python
		this->get_override("visit")(ScriptEntityClass(eclass));
	}
};

/**
 * greebo: This class provides the script interface for the GlobalEntityClassManager module.
 */
class EClassManagerInterface :
	public IScriptInterface
{
	IModelDef _emptyModelDef;

public:
	ScriptEntityClass findClass(const std::string& name) {
		// Find the eclass and convert implicitly to ScriptEntityClass
		return GlobalEntityClassManager().findClass(name);
	}

	IModelDef findModel(const std::string& name) {
		IModelDefPtr modelDef = GlobalEntityClassManager().findModel(name);
		return (modelDef != NULL) ? *modelDef : _emptyModelDef;
	}

	void forEach(EntityClassVisitor& visitor) {
		GlobalEntityClassManager().forEach(visitor);
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

		// Declare the Anims std::map to Python
		boost::python::class_<IModelDef::Anims>("Anims")
			.def(boost::python::map_indexing_suite<IModelDef::Anims, true>())
		;

		// Add the declaration for a ModelDef
		nspace["ModelDef"] = boost::python::class_<IModelDef>("ModelDef")
			.def_readonly("name", &IModelDef::name)
			.def_readonly("mesh", &IModelDef::mesh)
			.def_readonly("skin", &IModelDef::skin)
			.def_readonly("parent", &IModelDef::parent)
			.def_readonly("anims", &IModelDef::anims)
		;

		// Add the declaration for an EntityClass
		nspace["EntityClass"] = boost::python::class_<ScriptEntityClass>(
			"EntityClass", boost::python::init<const IEntityClassPtr&>())
			.def("getAttribute", &ScriptEntityClass::getAttribute, 
				boost::python::return_value_policy<
					boost::python::reference_existing_object,
					boost::python::default_call_policies>())
		;

		// Expose the entityclass visitor interface
		nspace["EntityClassVisitor"] = 
			boost::python::class_<EntityClassVisitorWrapper, boost::noncopyable>("EntityClassVisitor")
			.def("visit", boost::python::pure_virtual(&EntityClassVisitor::visit))
		;

		// Add the module declaration to the given python namespace
		nspace["GlobalEntityClassManager"] = boost::python::class_<EClassManagerInterface>("GlobalEntityClassManager")
			.def("findClass", &EClassManagerInterface::findClass)
			.def("forEach", &EClassManagerInterface::forEach)
			.def("findModel", &EClassManagerInterface::findModel)
		;

		// Now point the Python variable "GlobalEntityClassManager" to this instance
		nspace["GlobalEntityClassManager"] = boost::python::ptr(this);
	}
};
typedef boost::shared_ptr<EClassManagerInterface> EClassManagerInterfacePtr;

} // namespace script

#endif /* _ECLASS_INTERFACE_H_ */

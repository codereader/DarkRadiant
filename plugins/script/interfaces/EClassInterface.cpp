#include "EClassInterface.h"

#include <boost/python/suite/indexing/map_indexing_suite.hpp>

namespace script {

ScriptEntityClass EClassManagerInterface::findClass(const std::string& name) {
	// Find the eclass and convert implicitly to ScriptEntityClass
	return ScriptEntityClass(GlobalEntityClassManager().findClass(name));
}

IModelDef EClassManagerInterface::findModel(const std::string& name) {
	IModelDefPtr modelDef = GlobalEntityClassManager().findModel(name);
	return (modelDef != NULL) ? *modelDef : _emptyModelDef;
}

void EClassManagerInterface::forEachEntityClass(EntityClassVisitor& visitor)
{
	GlobalEntityClassManager().forEachEntityClass(visitor);
}

void EClassManagerInterface::forEachModelDef(ModelDefVisitor& visitor)
{
	GlobalEntityClassManager().forEachModelDef(visitor);
}

// IScriptInterface implementation
void EClassManagerInterface::registerInterface(boost::python::object& nspace) {
	// Add the declaration for the EClassAttribute
	nspace["EntityClassAttribute"] = boost::python::class_<EntityClassAttribute>(
		"EntityClassAttribute", boost::python::init<const EntityClassAttribute&>())
		.def("getType", &EntityClassAttribute::getType,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("getName", &EntityClassAttribute::getName,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("getValue", &EntityClassAttribute::getValue,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("getDescription", &EntityClassAttribute::getDescription,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def_readonly("inherited", &EntityClassAttribute::inherited)
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
		.def("isNull", &ScriptEntityClass::isNull)
		.def("isOfType", &ScriptEntityClass::isOfType)
		.def("getAttribute", &ScriptEntityClass::getAttribute,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
	;

	// Expose the entityclass visitor interface
	nspace["EntityClassVisitor"] =
		boost::python::class_<EntityClassVisitorWrapper, boost::noncopyable>("EntityClassVisitor")
		.def("visit", boost::python::pure_virtual(&EntityClassVisitor::visit))
	;

	// Expose the model def visitor interface
	nspace["ModelDefVisitor"] =
		boost::python::class_<ModelDefVisitorWrapper, boost::noncopyable>("ModelDefVisitor")
		.def("visit", boost::python::pure_virtual(&ModelDefVisitor::visit))
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalEntityClassManager"] = boost::python::class_<EClassManagerInterface>("GlobalEntityClassManager")
		.def("findClass", &EClassManagerInterface::findClass)
		.def("forEachEntityClass", &EClassManagerInterface::forEachEntityClass)
		.def("findModel", &EClassManagerInterface::findModel)
		.def("forEachModelDef", &EClassManagerInterface::forEachModelDef)
	;

	// Now point the Python variable "GlobalEntityClassManager" to this instance
	nspace["GlobalEntityClassManager"] = boost::python::ptr(this);
}

} // namespace script

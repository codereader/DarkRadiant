#include "EClassInterface.h"

#include <pybind11/stl_bind.h>

namespace script
{

ScriptEntityClass EClassManagerInterface::findClass(const std::string& name)
{
	// Find the eclass and convert implicitly to ScriptEntityClass
	return ScriptEntityClass(GlobalEntityClassManager().findClass(name));
}

IModelDef EClassManagerInterface::findModel(const std::string& name)
{
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
void EClassManagerInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the declaration for the EClassAttribute
	py::class_<EntityClassAttribute> eclassAttr(scope, "EntityClassAttribute");

	eclassAttr.def(py::init<const EntityClassAttribute&>());
	eclassAttr.def("getType", &EntityClassAttribute::getType, py::return_value_policy::reference);
	eclassAttr.def("getName", &EntityClassAttribute::getName, py::return_value_policy::reference);
	eclassAttr.def("getValue", &EntityClassAttribute::getValue, py::return_value_policy::reference);
	eclassAttr.def("getDescription", &EntityClassAttribute::getDescription, py::return_value_policy::reference);

	// Declare the Anims std::map to Python
	py::bind_map<IModelDef::Anims>(scope, "Anims");

	// Add the declaration for a ModelDef
	py::class_<IModelDef> modelDef(scope, "ModelDef");

	modelDef.def_readonly("name", &IModelDef::name);
	modelDef.def_readonly("mesh", &IModelDef::mesh);
	modelDef.def_readonly("skin", &IModelDef::skin);
	modelDef.def_readonly("parent", &IModelDef::parent);
	modelDef.def_readonly("anims", &IModelDef::anims);

	// Add the declaration for an EntityClass
	py::class_<ScriptEntityClass> eclass(scope, "EntityClass");

	eclass.def(py::init<const IEntityClassPtr&>());
	eclass.def("isNull", &ScriptEntityClass::isNull);
	eclass.def("isOfType", &ScriptEntityClass::isOfType);
	eclass.def("getAttributeValue", &ScriptEntityClass::getAttributeValue);
	eclass.def("getDefFileName", &ScriptEntityClass::getDefFileName);

	// Expose the entityclass visitor interface
	py::class_<EntityClassVisitor, EntityClassVisitorWrapper> eclassVisitor(scope, "EntityClassVisitor");
	eclassVisitor.def(py::init<>());
	eclassVisitor.def("visit", &EntityClassVisitor::visit);

	// Expose the model def visitor interface
	py::class_<ModelDefVisitor, ModelDefVisitorWrapper> modeldefVisitor(scope, "ModelDefVisitor");
	modeldefVisitor.def(py::init<>());
	modeldefVisitor.def("visit", &ModelDefVisitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<EClassManagerInterface> eclassManager(scope, "EntityClassManager");

	eclassManager.def("findClass", &EClassManagerInterface::findClass);
	eclassManager.def("forEachEntityClass", &EClassManagerInterface::forEachEntityClass);
	eclassManager.def("findModel", &EClassManagerInterface::findModel);
	eclassManager.def("forEachModelDef", &EClassManagerInterface::forEachModelDef);

	// Now point the Python variable "GlobalEntityClassManager" to this instance
	globals["GlobalEntityClassManager"] = this;
}

} // namespace script

#include "SelectionSetInterface.h"

namespace script
{

ScriptSelectionSet::ScriptSelectionSet(const selection::ISelectionSetPtr& set) :
	_set(set)
{}

const std::string& ScriptSelectionSet::getName()
{
	return (_set != NULL) ? _set->getName() : _emptyStr;
}

bool ScriptSelectionSet::empty()
{
	return (_set != NULL) ? _set->empty() : true;
}

void ScriptSelectionSet::select()
{
	if (_set != NULL) _set->select();
}

void ScriptSelectionSet::deselect()
{
	if (_set != NULL) _set->deselect();
}

void ScriptSelectionSet::clear()
{
	if (_set != NULL) _set->clear();
}

void ScriptSelectionSet::assignFromCurrentScene()
{
	if (_set != NULL) _set->assignFromCurrentScene();
}

std::string ScriptSelectionSet::_emptyStr;

// ---------------------------

void SelectionSetInterface::foreachSelectionSet(selection::ISelectionSetManager::Visitor& visitor)
{
	GlobalSelectionSetManager().foreachSelectionSet(visitor);
}

ScriptSelectionSet SelectionSetInterface::createSelectionSet(const std::string& name)
{
	return ScriptSelectionSet(GlobalSelectionSetManager().createSelectionSet(name));
}

void SelectionSetInterface::deleteSelectionSet(const std::string& name)
{
	GlobalSelectionSetManager().deleteSelectionSet(name);
}

void SelectionSetInterface::deleteAllSelectionSets()
{
	GlobalSelectionSetManager().deleteAllSelectionSets();
}

ScriptSelectionSet SelectionSetInterface::findSelectionSet(const std::string& name)
{
	return ScriptSelectionSet(GlobalSelectionSetManager().findSelectionSet(name));
}

// IScriptInterface implementation
void SelectionSetInterface::registerInterface(boost::python::object& nspace)
{
	// Expose the SelectionSystem::Visitor interface
	nspace["SelectionSetVisitor"] = boost::python::class_<SelectionSetVisitorWrapper, boost::noncopyable>("SelectionSetVisitor")
		.def("visit", boost::python::pure_virtual(&selection::ISelectionSetManager::Visitor::visit))
	;

	// Add SelectionSet declaration
	nspace["SelectionSet"] = boost::python::class_<ScriptSelectionSet>("SelectionSet",
		boost::python::init<const selection::ISelectionSetPtr&>())
		.def("getName", &ScriptSelectionSet::getName,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("empty", &ScriptSelectionSet::empty)
		.def("clear", &ScriptSelectionSet::clear)
		.def("select", &ScriptSelectionSet::select)
		.def("deselect", &ScriptSelectionSet::deselect)
		.def("assignFromCurrentScene", &ScriptSelectionSet::assignFromCurrentScene)
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalSelectionSetManager"] = boost::python::class_<SelectionSetInterface>("GlobalSelectionSetManager")
		.def("foreachSelectionSet", &SelectionSetInterface::foreachSelectionSet)
		.def("createSelectionSet", &SelectionSetInterface::createSelectionSet)
		.def("deleteSelectionSet", &SelectionSetInterface::deleteSelectionSet)
		.def("deleteAllSelectionSets", &SelectionSetInterface::deleteAllSelectionSets)
		.def("findSelectionSet", &SelectionSetInterface::findSelectionSet)
	;

	// Now point the Python variable "GlobalSelectionSetManager" to this instance
	nspace["GlobalSelectionSetManager"] = boost::python::ptr(this);
}

} // namespace script

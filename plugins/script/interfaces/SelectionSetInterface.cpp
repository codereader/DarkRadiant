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
void SelectionSetInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Expose the SelectionSystem::Visitor interface
	py::class_<selection::ISelectionSetManager::Visitor, SelectionSetVisitorWrapper> visitor(scope, "SelectionSetVisitor");

	visitor.def(py::init<>());
	visitor.def("visit", &selection::ISelectionSetManager::Visitor::visit);

	// Add SelectionSet declaration
	py::class_<ScriptSelectionSet> selectionSet(scope, "SelectionSet");

	selectionSet.def(py::init<const selection::ISelectionSetPtr&>());
	selectionSet.def("getName", &ScriptSelectionSet::getName, py::return_value_policy::reference);
	selectionSet.def("empty", &ScriptSelectionSet::empty);
	selectionSet.def("clear", &ScriptSelectionSet::clear);
	selectionSet.def("select", &ScriptSelectionSet::select);
	selectionSet.def("deselect", &ScriptSelectionSet::deselect);
	selectionSet.def("assignFromCurrentScene", &ScriptSelectionSet::assignFromCurrentScene);

	// Add the module declaration to the given python namespace
	py::class_<SelectionSetInterface> selectionSetManager(scope, "SelectionSetManager");

	selectionSetManager.def("foreachSelectionSet", &SelectionSetInterface::foreachSelectionSet);
	selectionSetManager.def("createSelectionSet", &SelectionSetInterface::createSelectionSet);
	selectionSetManager.def("deleteSelectionSet", &SelectionSetInterface::deleteSelectionSet);
	selectionSetManager.def("deleteAllSelectionSets", &SelectionSetInterface::deleteAllSelectionSets);
	selectionSetManager.def("findSelectionSet", &SelectionSetInterface::findSelectionSet);

	// Now point the Python variable "GlobalSelectionSetManager" to this instance
	globals["GlobalSelectionSetManager"] = this;
}

} // namespace script

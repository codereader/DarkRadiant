#include "SelectionSetInterface.h"

#include "imap.h"
#include "iselection.h"

namespace script
{

ScriptSelectionSet::ScriptSelectionSet(const selection::ISelectionSetPtr& set) :
	_set(set)
{}

const std::string& ScriptSelectionSet::getName()
{
	return (_set) ? _set->getName() : _emptyStr;
}

bool ScriptSelectionSet::empty()
{
	return (_set) ? _set->empty() : true;
}

void ScriptSelectionSet::select()
{
	if (_set) _set->select();
}

void ScriptSelectionSet::deselect()
{
	if (_set) _set->deselect();
}

void ScriptSelectionSet::clear()
{
	if (_set) _set->clear();
}

void ScriptSelectionSet::assignFromCurrentScene()
{
	if (_set) _set->assignFromCurrentScene();
}

std::string ScriptSelectionSet::_emptyStr;

// ---------------------------

inline selection::ISelectionSetManager& GetMapSelectionSetManager()
{
	if (!GlobalMapModule().getRoot())
	{
		throw std::runtime_error("No map loaded.");
	}

	return GlobalMapModule().getRoot()->getSelectionSetManager();
}

void SelectionSetInterface::foreachSelectionSet(selection::ISelectionSetManager::Visitor& visitor)
{
	try
	{
		GetMapSelectionSetManager().foreachSelectionSet(visitor);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

ScriptSelectionSet SelectionSetInterface::createSelectionSet(const std::string& name)
{
	try
	{
		return ScriptSelectionSet(GetMapSelectionSetManager().createSelectionSet(name));
	}
	catch (const std::runtime_error & ex)
	{
		rError() << ex.what() << std::endl;
		return ScriptSelectionSet(selection::ISelectionSetPtr());
	}
}

void SelectionSetInterface::deleteSelectionSet(const std::string& name)
{
	try
	{
		GetMapSelectionSetManager().deleteSelectionSet(name);
	}
	catch (const std::runtime_error & ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void SelectionSetInterface::deleteAllSelectionSets()
{
	try
	{
		GetMapSelectionSetManager().deleteAllSelectionSets();
	}
	catch (const std::runtime_error & ex)
	{
		rError() << ex.what() << std::endl;
	}
}

ScriptSelectionSet SelectionSetInterface::findSelectionSet(const std::string& name)
{
	try
	{
		return ScriptSelectionSet(GetMapSelectionSetManager().findSelectionSet(name));
	}
	catch (const std::runtime_error & ex)
	{
		rError() << ex.what() << std::endl;
		return ScriptSelectionSet(selection::ISelectionSetPtr());
	}
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

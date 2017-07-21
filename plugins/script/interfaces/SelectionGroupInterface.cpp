#include "SelectionGroupInterface.h"

#include <pybind11/pybind11.h>

namespace script
{

ScriptSelectionGroup::ScriptSelectionGroup(const selection::ISelectionGroupPtr& group) :
	_group(group)
{}

std::size_t ScriptSelectionGroup::getId()
{
	return _group ? _group->getId() : 0;
}

const std::string& ScriptSelectionGroup::getName()
{
	return _group ? _group->getName() : _emptyStr;
}

void ScriptSelectionGroup::setName(const std::string& name)
{
	if (_group)
	{
		_group->setName(name);
	}
}

void ScriptSelectionGroup::addNode(const ScriptSceneNode& node)
{
	if (_group)
	{
		_group->addNode(node);
	}
}

void ScriptSelectionGroup::removeNode(const scene::INodePtr& node)
{
	if (_group)
	{
		_group->removeNode(node);
	}
}

std::size_t ScriptSelectionGroup::size()
{
	return _group ? _group->size() : 0;
}

void ScriptSelectionGroup::setSelected(int selected)
{
	if (_group)
	{
		_group->setSelected(static_cast<bool>(selected));
	}
}

void ScriptSelectionGroup::foreachNode(SelectionGroupVisitor& visitor)
{
	if (_group)
	{
		_group->foreachNode([&](const scene::INodePtr& node)
		{
			visitor.visit(node);
		});
	}
}

std::string ScriptSelectionGroup::_emptyStr;

// -----------------------------------

ScriptSelectionGroup SelectionGroupInterface::createSelectionGroup()
{
	return ScriptSelectionGroup(GlobalSelectionGroupManager().createSelectionGroup());
}

ScriptSelectionGroup SelectionGroupInterface::getSelectionGroup(std::size_t id)
{
	return ScriptSelectionGroup(GlobalSelectionGroupManager().getSelectionGroup(id));
}

ScriptSelectionGroup SelectionGroupInterface::findOrCreateSelectionGroup(std::size_t id)
{
	return ScriptSelectionGroup(GlobalSelectionGroupManager().findOrCreateSelectionGroup(id));
}

void SelectionGroupInterface::setGroupSelected(std::size_t id, int selected)
{
	GlobalSelectionGroupManager().setGroupSelected(id, static_cast<bool>(selected));
}

void SelectionGroupInterface::deleteAllSelectionGroups()
{
	GlobalSelectionGroupManager().deleteAllSelectionGroups();
}

void SelectionGroupInterface::deleteSelectionGroup(std::size_t id)
{
	GlobalSelectionGroupManager().deleteSelectionGroup(id);
}

// IScriptInterface implementation
void SelectionGroupInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Expose the SelectionGroupVisitor interface
	py::class_<SelectionGroupVisitor, SelectionGroupVisitorWrapper> visitor(scope, "SelectionGroupVisitor");

	visitor.def(py::init<>());
	visitor.def("visit", &SelectionGroupVisitor::visit);

	// Add SelectionGroup declaration
	py::class_<ScriptSelectionGroup> selectionGroup(scope, "SelectionGroup");

	selectionGroup.def(py::init<const selection::ISelectionGroupPtr&>());
	selectionGroup.def("getId", &ScriptSelectionGroup::getId);
	selectionGroup.def("getName", &ScriptSelectionGroup::getName, py::return_value_policy::reference);
	selectionGroup.def("setName", &ScriptSelectionGroup::setName);
	selectionGroup.def("addNode", &ScriptSelectionGroup::addNode);
	selectionGroup.def("removeNode", &ScriptSelectionGroup::removeNode);
	selectionGroup.def("size", &ScriptSelectionGroup::size);
	selectionGroup.def("setSelected", &ScriptSelectionGroup::setSelected);
	selectionGroup.def("foreachNode", &ScriptSelectionGroup::foreachNode);

	// Add the module declaration to the given python namespace
	py::class_<SelectionGroupInterface> selectionGroupManager(scope, "SelectionGroupManager");

	selectionGroupManager.def("createSelectionGroup", &SelectionGroupInterface::createSelectionGroup);
	selectionGroupManager.def("getSelectionGroup", &SelectionGroupInterface::getSelectionGroup);
	selectionGroupManager.def("findOrCreateSelectionGroup", &SelectionGroupInterface::findOrCreateSelectionGroup);
	selectionGroupManager.def("setGroupSelected", &SelectionGroupInterface::setGroupSelected);
	selectionGroupManager.def("deleteAllSelectionGroups", &SelectionGroupInterface::deleteAllSelectionGroups);
	selectionGroupManager.def("deleteSelectionGroup", &SelectionGroupInterface::deleteSelectionGroup);

	// Now point the Python variable "GlobalSelectionGroupManager" to this instance
	globals["GlobalSelectionGroupManager"] = this;
}


}

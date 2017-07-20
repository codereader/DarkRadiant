#include "SelectionInterface.h"

namespace script 
{

const SelectionInfo& SelectionInterface::getSelectionInfo() 
{
	return GlobalSelectionSystem().getSelectionInfo();
}

void SelectionInterface::foreachSelected(const SelectionSystem::Visitor& visitor)
{
	GlobalSelectionSystem().foreachSelected(visitor);
}

void SelectionInterface::foreachSelectedComponent(const SelectionSystem::Visitor& visitor)
{
	GlobalSelectionSystem().foreachSelectedComponent(visitor);
}

void SelectionInterface::setSelectedAll(bool selected)
{
	GlobalSelectionSystem().setSelectedAll(selected);
}

void SelectionInterface::setSelectedAllComponents(bool selected)
{
	GlobalSelectionSystem().setSelectedAllComponents(selected);
}

ScriptSceneNode SelectionInterface::ultimateSelected()
{
	return GlobalSelectionSystem().ultimateSelected();
}

ScriptSceneNode SelectionInterface::penultimateSelected() 
{
	return GlobalSelectionSystem().penultimateSelected();
}

// IScriptInterface implementation
void SelectionInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Expose the SelectionInfo structure
	py::class_<SelectionInfo> selInfo(scope, "SelectionInformation");
	selInfo.def(py::init<>());
	selInfo.def_readonly("totalCount", &SelectionInfo::totalCount);
	selInfo.def_readonly("patchCount", &SelectionInfo::patchCount);
	selInfo.def_readonly("brushCount", &SelectionInfo::brushCount);
	selInfo.def_readonly("entityCount", &SelectionInfo::entityCount);
	selInfo.def_readonly("componentCount", &SelectionInfo::componentCount);

	// Expose the SelectionSystem::Visitor interface
	py::class_<SelectionSystem::Visitor, SelectionVisitorWrapper> visitor(scope, "SelectionVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &SelectionSystem::Visitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<SelectionInterface> selSys(scope, "SelectionSystem");

	selSys.def("getSelectionInfo", &SelectionInterface::getSelectionInfo, py::return_value_policy::reference);
	selSys.def("foreachSelected", &SelectionInterface::foreachSelected);
	selSys.def("foreachSelectedComponent", &SelectionInterface::foreachSelectedComponent);
	selSys.def("setSelectedAll", &SelectionInterface::setSelectedAll);
	selSys.def("setSelectedAllComponents", &SelectionInterface::setSelectedAllComponents);
	selSys.def("ultimateSelected", &SelectionInterface::ultimateSelected);
	selSys.def("penultimateSelected", &SelectionInterface::penultimateSelected);

	// Now point the Python variable "GlobalSelectionSystem" to this instance
	globals["GlobalSelectionSystem"] = this;
}

} // namespace script

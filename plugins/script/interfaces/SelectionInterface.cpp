#include "SelectionInterface.h"

namespace script {

const SelectionInfo& SelectionInterface::getSelectionInfo() {
	return GlobalSelectionSystem().getSelectionInfo();
}

void SelectionInterface::foreachSelected(const SelectionSystem::Visitor& visitor) {
	GlobalSelectionSystem().foreachSelected(visitor);
}

void SelectionInterface::foreachSelectedComponent(const SelectionSystem::Visitor& visitor) {
	GlobalSelectionSystem().foreachSelectedComponent(visitor);
}

void SelectionInterface::setSelectedAll(bool selected) {
	GlobalSelectionSystem().setSelectedAll(selected);
}

void SelectionInterface::setSelectedAllComponents(bool selected) {
	GlobalSelectionSystem().setSelectedAllComponents(selected);
}

ScriptSceneNode SelectionInterface::ultimateSelected() {
	return GlobalSelectionSystem().ultimateSelected();
}

ScriptSceneNode SelectionInterface::penultimateSelected() {
	return GlobalSelectionSystem().penultimateSelected();
}

// IScriptInterface implementation
void SelectionInterface::registerInterface(boost::python::object& nspace) {
	// Expose the SelectionInfo structure
	nspace["SelectionInfo"] = boost::python::class_<SelectionInfo>("SelectionInfo", boost::python::init<>())
		.def_readonly("totalCount", &SelectionInfo::totalCount)
		.def_readonly("patchCount", &SelectionInfo::patchCount)
		.def_readonly("brushCount", &SelectionInfo::brushCount)
		.def_readonly("entityCount", &SelectionInfo::entityCount)
		.def_readonly("componentCount", &SelectionInfo::componentCount)
	;

	// Expose the SelectionSystem::Visitor interface
	nspace["SelectionVisitor"] = boost::python::class_<SelectionVisitorWrapper, boost::noncopyable>("SelectionVisitor")
		.def("visit", boost::python::pure_virtual(&SelectionSystem::Visitor::visit))
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalSelectionSystem"] = boost::python::class_<SelectionInterface>("GlobalSelectionSystem")
		.def("getSelectionInfo", &SelectionInterface::getSelectionInfo,
			boost::python::return_value_policy<boost::python::copy_const_reference>())
		.def("foreachSelected", &SelectionInterface::foreachSelected)
		.def("foreachSelectedComponent", &SelectionInterface::foreachSelectedComponent)
		.def("setSelectedAll", &SelectionInterface::setSelectedAll)
		.def("setSelectedAllComponents", &SelectionInterface::setSelectedAllComponents)
		.def("ultimateSelected", &SelectionInterface::ultimateSelected)
		.def("penultimateSelected", &SelectionInterface::penultimateSelected)
	;

	// Now point the Python variable "GlobalSelectionSystem" to this instance
	nspace["GlobalSelectionSystem"] = boost::python::ptr(this);
}

} // namespace script

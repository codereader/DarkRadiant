#ifndef _SELECTION_INTERFACE_H_
#define _SELECTION_INTERFACE_H_

#include "iscript.h"
#include "selectionlib.h"
#include <map>

#include "SceneGraphInterface.h"

#include <boost/python.hpp>

namespace script {

// ========== Selection Handling ==========

// Wrap around the SelectionSystem::Visitor interface
class SelectionVisitorWrapper : 
	public SelectionSystem::Visitor, 
	public boost::python::wrapper<SelectionSystem::Visitor>
{
public:
    void visit(const scene::INodePtr& node) const {
		// Wrap this method to python
		this->get_override("visit")(ScriptSceneNode(node));
	}
};

class SelectionInterface :
	public IScriptInterface
{
public:
	const SelectionInfo& getSelectionInfo() {
		return GlobalSelectionSystem().getSelectionInfo();
	}

	void foreachSelected(const SelectionSystem::Visitor& visitor) {
		GlobalSelectionSystem().foreachSelected(visitor);
	}

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace) {
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
		;

		// Now point the Python variable "GlobalSelectionSystem" to this instance
		nspace["GlobalSelectionSystem"] = boost::python::ptr(this);
	}
};
typedef boost::shared_ptr<SelectionInterface> SelectionInterfacePtr;

} // namespace script

#endif /* _SELECTION_INTERFACE_H_ */

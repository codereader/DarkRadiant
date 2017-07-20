#pragma once

#include <pybind11/pybind11.h>

#include "iscript.h"
#include "selectionlib.h"
#include <map>

#include "SceneGraphInterface.h"

namespace script 
{

// ========== Selection Handling ==========

// Wrap around the SelectionSystem::Visitor interface
class SelectionVisitorWrapper :
	public SelectionSystem::Visitor
{
public:
    void visit(const scene::INodePtr& node) const override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			SelectionSystem::Visitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			ScriptSceneNode(node)			/* Argument(s) */
		);
	}
};

class SelectionInterface :
	public IScriptInterface
{
public:
	// SelectionSystem wrappers
	const SelectionInfo& getSelectionInfo();

	void foreachSelected(const SelectionSystem::Visitor& visitor);
	void foreachSelectedComponent(const SelectionSystem::Visitor& visitor);

	void setSelectedAll(bool selected);
	void setSelectedAllComponents(bool selected);

	ScriptSceneNode ultimateSelected();
	ScriptSceneNode penultimateSelected();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script

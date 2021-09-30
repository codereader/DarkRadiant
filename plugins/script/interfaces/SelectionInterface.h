#pragma once

#include <pybind11/pybind11.h>

#include "iscript.h"
#include "iscriptinterface.h"
#include "selectionlib.h"
#include <map>

#include "SceneGraphInterface.h"
#include "BrushInterface.h"

namespace script 
{

// ========== Selection Handling ==========

// Wrap around the SelectionSystem::Visitor interface
class SelectionVisitorWrapper :
    public selection::SelectionSystem::Visitor
{
public:
    void visit(const scene::INodePtr& node) const override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			selection::SelectionSystem::Visitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			ScriptSceneNode(node)			/* Argument(s) */
		);
	}
};

// Special interface only used by Python scripts to visit selected faces
class SelectedFaceVisitor
{
public:
    virtual ~SelectedFaceVisitor() {}

    virtual void visitFace(IFace& face) = 0;
};

class SelectedFaceVisitorWrapper :
    public SelectedFaceVisitor
{
public:
    void visitFace(IFace& face) override
    {
        // Wrap this method to python
        PYBIND11_OVERLOAD_PURE(
            void,			/* Return type */
            SelectedFaceVisitor,    /* Parent class */
            visitFace,			/* Name of function in C++ (must match Python name) */
            ScriptFace(face)			/* Argument(s) */
        );
    }
};

class SelectionInterface :
	public IScriptInterface
{
public:
	// SelectionSystem wrappers
	const SelectionInfo& getSelectionInfo();

	void foreachSelected(const selection::SelectionSystem::Visitor& visitor);
	void foreachSelectedComponent(const selection::SelectionSystem::Visitor& visitor);
	void foreachSelectedFace(SelectedFaceVisitor& visitor);

	void setSelectedAll(int selected);
	void setSelectedAllComponents(int selected);

	ScriptSceneNode ultimateSelected();
	ScriptSceneNode penultimateSelected();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script

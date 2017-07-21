#pragma once

#include <pybind11/pybind11.h>

#include "iscript.h"
#include "iselectiongroup.h"

#include "SceneGraphInterface.h"

namespace script
{

// ========== SelectionGroup Handling ==========

class SelectionGroupVisitor
{
public:
	virtual ~SelectionGroupVisitor() {}
	virtual void visit(const scene::INodePtr& node) = 0;
};

// Wrap around the ISelectionSetManager::Visitor interface
class SelectionGroupVisitorWrapper :
	public SelectionGroupVisitor
{
public:
	void visit(const scene::INodePtr& node) override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			SelectionGroupVisitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			ScriptSceneNode(node)			/* Argument(s) */
		);
	}
};

class ScriptSelectionGroup
{
private:
	selection::ISelectionGroupPtr _group;

	static std::string _emptyStr;
public:
	ScriptSelectionGroup(const selection::ISelectionGroupPtr& group);

	std::size_t getId();
	const std::string& getName();
	void setName(const std::string& name);
	void addNode(const ScriptSceneNode& node);
	void removeNode(const scene::INodePtr& node);
	std::size_t size();
	void setSelected(int selected);
	void foreachNode(SelectionGroupVisitor& visitor);
};

class SelectionGroupInterface :
	public IScriptInterface
{
public:
	ScriptSelectionGroup createSelectionGroup();
	ScriptSelectionGroup getSelectionGroup(std::size_t id);
	ScriptSelectionGroup findOrCreateSelectionGroup(std::size_t id);
	void setGroupSelected(std::size_t id, int selected);
	void deleteAllSelectionGroups();
	void deleteSelectionGroup(std::size_t id);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script

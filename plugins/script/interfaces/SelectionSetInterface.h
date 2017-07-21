#pragma once

#include <pybind11/pybind11.h>

#include "iscript.h"
#include "iselectionset.h"

namespace script
{

// ========== SelectionSet Handling ==========

class ScriptSelectionSet
{
private:
	selection::ISelectionSetPtr _set;

	static std::string _emptyStr;
public:
	ScriptSelectionSet(const selection::ISelectionSetPtr& set);

	const std::string& getName();
	bool empty();
	void select();
	void deselect();
	void clear();
	void assignFromCurrentScene();
};

// Wrap around the ISelectionSetManager::Visitor interface
class SelectionSetVisitorWrapper :
	public selection::ISelectionSetManager::Visitor
{
public:
    void visit(const selection::ISelectionSetPtr& set) override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			selection::ISelectionSetManager::Visitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			ScriptSelectionSet(set)			/* Argument(s) */
		);
	}
};

class SelectionSetInterface :
	public IScriptInterface
{
public:
	// SelectionSetManager wrappers
	void foreachSelectionSet(selection::ISelectionSetManager::Visitor& visitor);
	ScriptSelectionSet createSelectionSet(const std::string& name);
	void deleteSelectionSet(const std::string& name);
	void deleteAllSelectionSets();
	ScriptSelectionSet findSelectionSet(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script

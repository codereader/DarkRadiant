#ifndef _SELECTION_SET_INTERFACE_H_
#define _SELECTION_SET_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"
#include "iselectionset.h"

namespace script {

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
	public selection::ISelectionSetManager::Visitor,
	public boost::python::wrapper<selection::ISelectionSetManager::Visitor>
{
public:
    void visit(const selection::ISelectionSetPtr& set)
	{
		// Wrap this method to python
		this->get_override("visit")(ScriptSelectionSet(set));
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
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<SelectionSetInterface> SelectionSetInterfacePtr;

} // namespace script

#endif /* _SELECTION_SET_INTERFACE_H_ */

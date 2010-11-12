#ifndef _SELECTION_INTERFACE_H_
#define _SELECTION_INTERFACE_H_

#include <boost/python.hpp>

#include "iscript.h"
#include "selectionlib.h"
#include <map>

#include "SceneGraphInterface.h"

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
	// SelectionSystem wrappers
	const SelectionInfo& getSelectionInfo();

	void foreachSelected(const SelectionSystem::Visitor& visitor);
	void foreachSelectedComponent(const SelectionSystem::Visitor& visitor);

	void setSelectedAll(bool selected);
	void setSelectedAllComponents(bool selected);

	ScriptSceneNode ultimateSelected();
	ScriptSceneNode penultimateSelected();

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<SelectionInterface> SelectionInterfacePtr;

} // namespace script

#endif /* _SELECTION_INTERFACE_H_ */

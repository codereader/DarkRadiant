#ifndef _SCENEGRAPH_INTERFACE_H_
#define _SCENEGRAPH_INTERFACE_H_

#include "inode.h"
#include "iscript.h"
#include "scenelib.h"

#include <boost/python.hpp>

namespace script {

class ScriptSceneNode
{
	// The contained scene::INodePtr
	const scene::INodePtr _node;

	AABB _emptyAABB;
public:
	ScriptSceneNode(const scene::INodePtr& node) :
		_node(node)
	{}

	operator const scene::INodePtr&() const {
		return _node;
	}

	void removeFromParent() {
		if (_node != NULL) {
			scene::removeNodeFromParent(_node);
		}
	}

	void addToContainer(const ScriptSceneNode& container) {
		if (_node != NULL) {
			scene::addNodeToContainer(_node, container);
		}
	}

	const AABB& getWorldAABB() const {
		return _node != NULL ? _node->worldAABB() : _emptyAABB;
	}

	bool isNull() const {
		return _node == NULL;
	}

	ScriptSceneNode getParent() {
		return _node != NULL ? _node->getParent() : ScriptSceneNode(scene::INodePtr());
	}

	std::string getNodeType() {
		return nodetype_get_name(node_get_nodetype(_node));
	}
};

class SceneGraphInterface :
	public IScriptInterface
{
public:
	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace) {
		// Expose the scene::Node interface
		nspace["SceneNode"] = boost::python::class_<ScriptSceneNode>(
			"SceneNode", boost::python::init<const scene::INodePtr&>())
			.def("addToContainer", &ScriptSceneNode::addToContainer)
			.def("removeFromParent", &ScriptSceneNode::removeFromParent)
			.def("getWorldAABB", &ScriptSceneNode::getWorldAABB,
				boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("isNull", &ScriptSceneNode::isNull)
			.def("getParent", &ScriptSceneNode::getParent)
			.def("getNodeType", &ScriptSceneNode::getNodeType)
		;
	}
};
typedef boost::shared_ptr<SceneGraphInterface> SceneGraphInterfacePtr;

} // namespace script

#endif /* _SCENEGRAPH_INTERFACE_H_ */

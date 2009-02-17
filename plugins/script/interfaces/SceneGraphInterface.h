#ifndef _SCENEGRAPH_INTERFACE_H_
#define _SCENEGRAPH_INTERFACE_H_

#include "inode.h"
#include "iscript.h"
#include "iscenegraph.h"
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

	void traverse(scene::NodeVisitor& visitor) {
		if (_node != NULL) {
			_node->traverse(visitor);
		}
	}
};

// Wrap around the scene::NodeVisitor interface
class SceneNodeVisitorWrapper : 
	public scene::NodeVisitor, 
	public boost::python::wrapper<scene::NodeVisitor>
{
public:
    bool pre(const scene::INodePtr& node) {
		// Wrap this method to python
		return this->get_override("pre")(ScriptSceneNode(node));
	}

	void post(const scene::INodePtr& node) {
		if (this->get_override("post")) {
			// Call the overriden method
            this->get_override("post")(ScriptSceneNode(node)); 
		}
		else {
			// No override, call base class default
			scene::NodeVisitor::post(node);
		}
	}

	void post_default(const scene::INodePtr& node) {
		// Default method: Just call the base class
		scene::NodeVisitor::post(node);
	}
};

class SceneGraphInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode root() {
		return GlobalSceneGraph().root();
	}

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
			.def("traverse", &ScriptSceneNode::traverse)
		;

		// Expose the scene::NodeVisitor interface
		nspace["SceneNodeVisitor"] = 
			boost::python::class_<SceneNodeVisitorWrapper, boost::noncopyable>("SceneNodeVisitor")
			.def("pre", boost::python::pure_virtual(&scene::NodeVisitor::pre))
			.def("post", &scene::NodeVisitor::post, &SceneNodeVisitorWrapper::post_default) // respect default impl.
		;

		// Add the module declaration to the given python namespace
		nspace["GlobalSceneGraph"] = boost::python::class_<SceneGraphInterface>("GlobalSceneGraph")
			.def("root", &SceneGraphInterface::root)
		;

		// Now point the Python variable "GlobalSceneGraph" to this instance
		nspace["GlobalSceneGraph"] = boost::python::ptr(this);
	}
};
typedef boost::shared_ptr<SceneGraphInterface> SceneGraphInterfacePtr;

} // namespace script

#endif /* _SCENEGRAPH_INTERFACE_H_ */

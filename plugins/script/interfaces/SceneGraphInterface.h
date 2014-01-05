#pragma once

#include "inode.h"
#include "iscript.h"
#include "iscenegraph.h"
#include "scenelib.h"
#include "iselection.h"
#include "debugging/ScenegraphUtils.h"

#include <boost/python.hpp>

namespace script {

class ScriptSceneNode
{
protected:
	// The contained scene::INodePtr
	const scene::INodeWeakPtr _node;

	AABB _emptyAABB;
public:
	ScriptSceneNode(const scene::INodePtr& node) :
		_node(node)
	{}

	virtual ~ScriptSceneNode()
	{}

	operator scene::INodePtr() const {
		return _node.lock();
	}

	void removeFromParent() {
		scene::INodePtr node = _node.lock();
		if (node != NULL) {
			scene::removeNodeFromParent(node);
		}
	}

	void addToContainer(const ScriptSceneNode& container) {
		scene::INodePtr node = _node.lock();
		if (node != NULL) {
			scene::addNodeToContainer(node, container);
		}
	}

	const AABB& getWorldAABB() const {
		scene::INodePtr node = _node.lock();
		return node != NULL ? node->worldAABB() : _emptyAABB;
	}

	bool isNull() const {
		return _node.lock() == NULL;
	}

	ScriptSceneNode getParent() {
		scene::INodePtr node = _node.lock();
		return node != NULL
                    ? ScriptSceneNode(node->getParent())
                    : ScriptSceneNode(scene::INodePtr());
	}

	std::string getNodeType() {
		scene::INodePtr node = _node.lock();
		return node != NULL ? getNameForNodeType(node->getNodeType()) : "null";
	}

	void traverse(scene::NodeVisitor& visitor) {
		scene::INodePtr node = _node.lock();
		if (node != NULL) {
			node->traverse(visitor);
		}
	}

	void traverseChildren(scene::NodeVisitor& visitor) {
		scene::INodePtr node = _node.lock();
		if (node != NULL) {
			node->traverseChildren(visitor);
		}
	}

	bool isSelected() {
		scene::INodePtr node = _node.lock();
		if (node == NULL) return false;

		SelectablePtr selectable = Node_getSelectable(node);

		return (selectable != NULL) ? selectable->isSelected() : false;
	}

	void setSelected(bool selected) {
		scene::INodePtr node = _node.lock();
		if (node == NULL) return;

		SelectablePtr selectable = Node_getSelectable(node);

		if (selectable != NULL) {
			selectable->setSelected(selected);
		}
	}

	void invertSelected(bool selected) {
		scene::INodePtr node = _node.lock();
		if (node == NULL) return;

		SelectablePtr selectable = Node_getSelectable(node);

		if (selectable != NULL) {
			selectable->invertSelected();
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
			.def("traverseChildren", &ScriptSceneNode::traverseChildren)
			.def("setSelected", &ScriptSceneNode::setSelected)
			.def("invertSelected", &ScriptSceneNode::invertSelected)
			.def("isSelected", &ScriptSceneNode::isSelected)
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

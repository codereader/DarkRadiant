#pragma once

#include "inode.h"
#include "iscript.h"
#include "iscenegraph.h"
#include "scenelib.h"
#include "iselection.h"
#include "debugging/ScenegraphUtils.h"

#include <pybind11/pybind11.h>
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

		ISelectablePtr selectable = Node_getSelectable(node);

		return (selectable != NULL) ? selectable->isSelected() : false;
	}

	void setSelected(bool selected) {
		scene::INodePtr node = _node.lock();
		if (node == NULL) return;

		ISelectablePtr selectable = Node_getSelectable(node);

		if (selectable != NULL) {
			selectable->setSelected(selected);
		}
	}

	void invertSelected() {
		scene::INodePtr node = _node.lock();
		if (node == NULL) return;

		ISelectablePtr selectable = Node_getSelectable(node);

		if (selectable != NULL) {
			selectable->setSelected(!selectable->isSelected());
		}
	}
};

// Wrap around the scene::NodeVisitor interface
class SceneNodeVisitorWrapper :
	public scene::NodeVisitor,
	public boost::python::wrapper<scene::NodeVisitor>
{
public:
	using NodeVisitor::NodeVisitor;

    bool pre(const scene::INodePtr& node) override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			int,			/* Return type */
			NodeVisitor,    /* Parent class */
			pre,			/* Name of function in C++ (must match Python name) */
			ScriptSceneNode(node)			/* Argument(s) */
		);
	}

	void post(const scene::INodePtr& node) override
	{
		PYBIND11_OVERLOAD(
			void,			/* Return type */
			NodeVisitor,    /* Parent class */
			post,			/* Name of function in C++ (must match Python name) */
			ScriptSceneNode(node)			/* Argument(s) */
		);
	}
};

class SceneGraphInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode root() 
	{
		return ScriptSceneNode(GlobalSceneGraph().root());
	}

	void registerInterface(py::module& scope, py::dict& globals) override
	{
		// Expose the scene::Node interface
		py::class_<ScriptSceneNode> sceneNode(scope, "SceneNode");

		sceneNode.def(py::init<const scene::INodePtr&>());
		sceneNode.def("addToContainer", &ScriptSceneNode::addToContainer);
		sceneNode.def("removeFromParent", &ScriptSceneNode::removeFromParent);
		sceneNode.def("getWorldAABB", &ScriptSceneNode::getWorldAABB, py::return_value_policy::reference);
		sceneNode.def("isNull", &ScriptSceneNode::isNull);
		sceneNode.def("getParent", &ScriptSceneNode::getParent);
		sceneNode.def("getNodeType", &ScriptSceneNode::getNodeType);
		sceneNode.def("traverse", &ScriptSceneNode::traverse);
		sceneNode.def("traverseChildren", &ScriptSceneNode::traverseChildren);
		sceneNode.def("setSelected", &ScriptSceneNode::setSelected);
		sceneNode.def("invertSelected", &ScriptSceneNode::invertSelected);
		sceneNode.def("isSelected", &ScriptSceneNode::isSelected);

		py::class_<scene::NodeVisitor, SceneNodeVisitorWrapper> visitor(scope, "SceneNodeVisitor");
		visitor.def(py::init<>());
		visitor.def("pre", &scene::NodeVisitor::pre);
		visitor.def("post", &scene::NodeVisitor::post);

#if 0
		// Expose the scene::NodeVisitor interface
		nspace["SceneNodeVisitor"] =
			boost::python::class_<SceneNodeVisitorWrapper, boost::noncopyable>("SceneNodeVisitor")
			.def("pre", boost::python::pure_virtual(&scene::NodeVisitor::pre))
			.def("post", &scene::NodeVisitor::post, &SceneNodeVisitorWrapper::post_default) // respect default impl.
			;
#endif
		// Add the module declaration to the given python namespace
		py::class_<SceneGraphInterface> sceneGraphInterface(scope, "SceneGraph");
		sceneGraphInterface.def("root", &SceneGraphInterface::root);
		
		// Now point the Python variable "GlobalSceneGraph" to this instance
		globals["GlobalSceneGraph"] = this;
	}

#if 0
	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace)
	{
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
#endif
};
typedef std::shared_ptr<SceneGraphInterface> SceneGraphInterfacePtr;

} // namespace script

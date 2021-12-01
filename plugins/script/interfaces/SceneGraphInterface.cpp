#include "SceneGraphInterface.h"

#include "imodel.h"
#include "scenelib.h"
#include "iselection.h"
#include "debugging/ScenegraphUtils.h"

#include "ModelInterface.h"
#include "BrushInterface.h"
#include "EntityInterface.h"
#include "PatchInterface.h"

namespace script
{

ScriptSceneNode::ScriptSceneNode(const scene::INodePtr& node) :
	_node(node)
{}

ScriptSceneNode::~ScriptSceneNode()
{}

ScriptSceneNode::operator scene::INodePtr() const
{
	return _node.lock();
}

void ScriptSceneNode::removeFromParent()
{
	scene::INodePtr node = _node.lock();
	if (node != NULL) {
		scene::removeNodeFromParent(node);
	}
}

void ScriptSceneNode::addToContainer(const ScriptSceneNode& container)
{
	scene::INodePtr node = _node.lock();
	if (node != NULL) {
		scene::addNodeToContainer(node, container);
	}
}

const AABB& ScriptSceneNode::getWorldAABB() const
{
	scene::INodePtr node = _node.lock();
	return node != NULL ? node->worldAABB() : _emptyAABB;
}

bool ScriptSceneNode::isNull() const
{
	return _node.lock() == NULL;
}

ScriptSceneNode ScriptSceneNode::getParent()
{
	scene::INodePtr node = _node.lock();
	return node != NULL ? ScriptSceneNode(node->getParent()) : ScriptSceneNode(scene::INodePtr());
}

std::string ScriptSceneNode::getNodeType()
{
	scene::INodePtr node = _node.lock();
	return node != NULL ? getNameForNodeType(node->getNodeType()) : "null";
}

void ScriptSceneNode::traverse(scene::NodeVisitor& visitor)
{
	scene::INodePtr node = _node.lock();
	if (node != NULL) {
		node->traverse(visitor);
	}
}

void ScriptSceneNode::traverseChildren(scene::NodeVisitor& visitor)
{
	scene::INodePtr node = _node.lock();
	if (node != NULL) {
		node->traverseChildren(visitor);
	}
}

bool ScriptSceneNode::isSelected()
{
	scene::INodePtr node = _node.lock();
	if (node == NULL) return false;

	ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

	return (selectable != NULL) ? selectable->isSelected() : false;
}

void ScriptSceneNode::setSelected(int selected)
{
	scene::INodePtr node = _node.lock();
	if (node == NULL) return;

	ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

	if (selectable != NULL) {
		selectable->setSelected(static_cast<bool>(selected));
	}
}

void ScriptSceneNode::invertSelected()
{
	scene::INodePtr node = _node.lock();
	if (node == NULL) return;

	ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

	if (selectable != NULL) {
		selectable->setSelected(!selectable->isSelected());
	}
}

ScriptSceneNode SceneGraphInterface::root()
{
	return ScriptSceneNode(GlobalSceneGraph().root());
}

void SceneGraphInterface::registerInterface(py::module& scope, py::dict& globals)
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

	// Add the "isModel" and "getModel" methods to all ScriptSceneNodes
	sceneNode.def("isModel", &ScriptModelNode::isModel);
	sceneNode.def("getModel", &ScriptModelNode::getModel);

	// Add the "is/get" brush methods
	sceneNode.def("isBrush", &ScriptBrushNode::isBrush);
	sceneNode.def("getBrush", &ScriptBrushNode::getBrush);

	// Add the "is/get" entity methods
	sceneNode.def("isEntity", &ScriptEntityNode::isEntity);
	sceneNode.def("getEntity", &ScriptEntityNode::getEntity);

	// Add the "is/get" patch methods
	sceneNode.def("isPatch", &ScriptPatchNode::isPatch);
	sceneNode.def("getPatch", &ScriptPatchNode::getPatch);

	py::class_<scene::NodeVisitor, SceneNodeVisitorWrapper> visitor(scope, "SceneNodeVisitor");
	visitor.def(py::init<>());
	visitor.def("pre", &scene::NodeVisitor::pre);
	visitor.def("post", &scene::NodeVisitor::post);

	// Add the module declaration to the given python namespace
	py::class_<SceneGraphInterface> sceneGraphInterface(scope, "SceneGraph");
	sceneGraphInterface.def("root", &SceneGraphInterface::root);

	// Now point the Python variable "GlobalSceneGraph" to this instance
	globals["GlobalSceneGraph"] = this;
}

} // namespace script

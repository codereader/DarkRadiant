#pragma once

#include "inode.h"
#include "iscript.h"
#include "iscenegraph.h"
#include "math/AABB.h"

#include <pybind11/pybind11.h>

namespace script 
{

class ScriptSceneNode
{
protected:
	// The contained scene::INodePtr
	const scene::INodeWeakPtr _node;

	AABB _emptyAABB;
public:
	ScriptSceneNode(const scene::INodePtr& node);

	virtual ~ScriptSceneNode();

	operator scene::INodePtr() const;

	void removeFromParent();
	void addToContainer(const ScriptSceneNode& container);

	const AABB& getWorldAABB() const;

	bool isNull() const;

	ScriptSceneNode getParent();

	std::string getNodeType();

	void traverse(scene::NodeVisitor& visitor);

	void traverseChildren(scene::NodeVisitor& visitor);

	bool isSelected();

	void setSelected(int selected);

	void invertSelected();
};

// Wrap around the scene::NodeVisitor interface
class SceneNodeVisitorWrapper :
	public scene::NodeVisitor
{
public:
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
	ScriptSceneNode root();

	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script

#pragma once

#include "ipath.h"
#include "inode.h"
#include "math/AABB.h"
#include "iscenegraph.h"
#include "string/convert.h"
#include "itextstream.h"

inline std::string getNameForNodeType(scene::INode::Type type)
{
    switch (type)
    {
	case scene::INode::Type::MapRoot: return "map";
    case scene::INode::Type::Entity: return "entity";
    case scene::INode::Type::Primitive: return "primitive";
    case scene::INode::Type::Model: return "model";
    case scene::INode::Type::Particle: return "particle";
    default: return "unknown";
    };
}

// greebo: Return information about the given node
inline std::string getNodeInfo(const scene::INodePtr& node)
{
	std::string returnValue;

	if (node == NULL) {
		return "NULL";
	}

	returnValue += getNameForNodeType(node->getNodeType());
    returnValue += " (" + node->name() + ")";

	return returnValue;
}

// greebo: Stream insertion operator for scene::INode
inline std::ostream& operator<<(std::ostream& st, const scene::INodePtr& node) {
	st << getNodeInfo(node);
	return st;
}

inline std::string getPathInfo(const scene::Path& path) {
	std::string name;

	for (scene::Path::const_iterator i = path.begin(); i != path.end(); i++) {
		// Cast the INode onto a scene::Node
		scene::INodePtr node = *i;

		name += (name.empty()) ? "" : ", ";
		name += getNodeInfo(node);
	}
	name = std::string("[") + string::to_string(path.size()) + "] {" + name + "} extents: ";
	name += "<" + string::to_string(path.top()->worldAABB().extents.x()) + ",";
	name += string::to_string(path.top()->worldAABB().extents.y()) + ",";
	name += string::to_string(path.top()->worldAABB().extents.z()) + ">";
	return name;
}

// greebo: Stream-insertion operator for scene::Paths, printing out information on the referenced scene::Nodes
inline std::ostream& operator<<(std::ostream& st, const scene::Path& path) {
	st << getPathInfo(path);
	return st;
}

class SceneGraphDumper :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node)
	{
		rMessage() << getNodeInfo(node) << "\n";
		return true;
	}
};

inline void dumpSceneGraph() {
	SceneGraphDumper dumper;
	GlobalSceneGraph().root()->traverseChildren(dumper);
}

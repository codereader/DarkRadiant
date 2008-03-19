#ifndef SCENEGRAPHUTILS_H_
#define SCENEGRAPHUTILS_H_

#include "ipath.h"
#include "inode.h"
#include "nameable.h"
#include "scenelib.h"
#include "string/string.h"

// greebo: Return information about the given node
inline std::string getNodeInfo(const scene::INodePtr& node) {
	std::string returnValue;
	
	if (node == NULL) {
		return "NULL";
	}
	
	returnValue += (node->isRoot()) ? "root" : nodetype_get_name(node_get_nodetype(node));
	
	NameablePtr nameable = boost::dynamic_pointer_cast<Nameable>(node);
	if (nameable != NULL) {
		returnValue += " (" + nameable->name() + ")";
	}
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
	name = std::string("[") + sizetToStr(path.size()) + "] {" + name + "} extents: ";
	name += "<" + doubleToStr(path.top()->worldAABB().extents.x()) + ",";
	name += doubleToStr(path.top()->worldAABB().extents.y()) + ",";
	name += doubleToStr(path.top()->worldAABB().extents.z()) + ">";
	return name;
}

// greebo: Stream-insertion operator for scene::Paths, printing out information on the referenced scene::Nodes
inline std::ostream& operator<<(std::ostream& st, const scene::Path& path) {
	st << getPathInfo(path);
	return st;
}

class SceneGraphDumper :
	public scene::Graph::Walker
{
public:
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		globalOutputStream() << getPathInfo(path).c_str() << "\n";
		std::cout << getPathInfo(path) << "\n";
		return true;
	}
};

inline void dumpSceneGraph() {
	SceneGraphDumper dumper;
	GlobalSceneGraph().traverse(dumper);
}

#endif /*SCENEGRAPHUTILS_H_*/

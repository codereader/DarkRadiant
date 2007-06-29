#ifndef SCENEGRAPHUTILS_H_
#define SCENEGRAPHUTILS_H_

#include "ipath.h"
#include "nameable.h"
#include "scenelib.h"

// greebo: Return information about the given node
inline std::string getNodeInfo(const scene::INodePtr& node) {
	std::string returnValue;
	
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

// greebo: Stream-insertion operator for scene::Paths, printing out information on the referenced scene::Nodes
inline std::ostream& operator<<(std::ostream& st, const scene::Path& path) {
	std::string name;
	
	for (scene::Path::const_iterator i = path.begin(); i != path.end(); i++) {
		// Cast the INode onto a scene::Node
		scene::INodePtr node = *i;
		
		name += (name.empty()) ? "" : ", ";
		name += getNodeInfo(node);
	}
	st << "[" << path.size() << "] {" << name << "}";
	return st;
}

#endif /*SCENEGRAPHUTILS_H_*/

#ifndef MAP_EXPORT_INFO_H_
#define MAP_EXPORT_INFO_H_

#include <ostream>

namespace map {

/**
 * greebo: A MapExportInfo structure contains all the information
 *         needed to export a map file. 
 */
class MapExportInfo
{
public:
	// The starting node, which will be exported along with all its children.
	scene::INodePtr root;

	// A function pointer for traversing the scenegraph in a certain way.
	// This function may impose (filtering) rules on how to traverse the graph.
	GraphTraversalFunc traverse;

	// The output stream where the Doom3-compatible map file data 
	// will be written to.
	std::ostream& mapStream;

	// An auxiliary stream, where additional meta-data (like layer assignments, etc.)
	// can be written to.
	std::ostream& infoStream;

	MapExportInfo(std::ostream& mapStr, std::ostream& infoStr) :
		mapStream(mapStr),
		infoStream(infoStr)
	{}
};

} // namespace map

#endif /* MAP_EXPORT_INFO_H_ */

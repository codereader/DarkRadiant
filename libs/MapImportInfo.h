#ifndef MAP_IMPORT_INFO_H_
#define MAP_IMPORT_INFO_H_

#include "itextstream.h"
#include "parser/DefTokeniser.h"

namespace map {

/**
 * greebo: A MapExportInfo structure contains all the information
 *         needed to export a map file. 
 */
class MapImportInfo
{
public:
	// The root node will be the parent of the parsed nodes
	scene::INodePtr root;

	// The TextInputStream will refer to the actual Doom 3 map data
	TextInputStream& inputStream;

	// An auxiliary stream, where additional meta-data (like layer assignments, etc.)
	// can be read from.
	std::istream& infoStream;

	MapImportInfo(TextInputStream& istr, std::istream& infoStr) :
		inputStream(istr),
		infoStream(infoStr)
	{}
};

} // namespace map

#endif /* MAP_IMPORT_INFO_H_ */

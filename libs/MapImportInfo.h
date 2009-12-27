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

	// The input stream containing the actual Doom 3 map data
	std::istream& inputStream;

	// The number of bytes of the input stream
	long inputStreamSize;

	// An auxiliary stream, where additional meta-data (like layer assignments, etc.)
	// can be read from.
	std::istream& infoStream;

	MapImportInfo(std::istream& istr, std::istream& infoStr) :
		inputStream(istr),
		infoStream(infoStr)
	{
		// Get the file size
		inputStream.seekg(0, std::ios::end);
		inputStreamSize = static_cast<long>(inputStream.tellg());

		// Move the pointer back to the beginning of the file
		inputStream.seekg(0, std::ios::beg);
	}
};

} // namespace map

#endif /* MAP_IMPORT_INFO_H_ */

#if 0
//#ifndef MAP_IMPORT_INFO_H_
#define MAP_IMPORT_INFO_H_

#include "itextstream.h"
#include "parser/DefTokeniser.h"

#include <boost/bind.hpp>

namespace map {

typedef boost::function<void(std::size_t)> MapImportProgressFunc;

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

	// Callbacks to invoke as parsing progresses
	MapImportProgressFunc onEntityParsedCallback;

	// Callbacks to invoke as parsing progresses
	MapImportProgressFunc onPrimitiveParsedCallback;

	/**
	 * Map import info structure which is passed to any mapformat importer.
	 *
	 * The progress callbacks are optional, still it's safe for the map importer
	 * to invoke them without previous checks as they are pointed to an actual
	 * function in all cases, even if it's just a NOP.
	 */
	MapImportInfo(std::istream& istr, 
				  const scene::INodePtr& root_,
				  MapImportProgressFunc onEntityParsedCallback_ = MapImportProgressFunc(), 
				  MapImportProgressFunc onPrimitiveParsedCallback_ = MapImportProgressFunc()) :
		inputStream(istr),
		root(root_),
		onEntityParsedCallback(onEntityParsedCallback_),
		onPrimitiveParsedCallback(onPrimitiveParsedCallback_)
	{
		// Get the file size
		inputStream.seekg(0, std::ios::end);
		inputStreamSize = static_cast<long>(inputStream.tellg());

		// Move the pointer back to the beginning of the file
		inputStream.seekg(0, std::ios::beg);

		// Ensure we have valid callbacks
		if (!onEntityParsedCallback)
		{
			onEntityParsedCallback = nopProgress;
		}

		if (!onPrimitiveParsedCallback)
		{
			onPrimitiveParsedCallback = nopProgress;
		}
	}

private:
	static void nopProgress(std::size_t)
	{}
};

} // namespace map

#endif /* MAP_IMPORT_INFO_H_ */

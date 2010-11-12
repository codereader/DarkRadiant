#ifndef _imapformat_h__
#define _imapformat_h__

#include "imodule.h"

namespace scene
{
class NodeVisitor;
class INode;
typedef boost::shared_ptr<INode> INodePtr;
}

namespace parser { class DefTokeniser; }

/** Callback function to control how the Walker traverses the scene graph. This function
 * will be provided to the map export module by the Radiant map code.
 */
typedef void (*GraphTraversalFunc) (scene::INodePtr root, scene::NodeVisitor& nodeExporter);

namespace map
{

// The MapExport/MapImport information structures, needed by the MapFormat::write() and
// MapFormat::read() methods. The actual definition of this structure is stored in the
// files libs/MapExportInfo.h and libs/MapImportInfo.h.
class MapExportInfo;
class MapImportInfo;

/**
 * A Primitive parser is able to create a primitive (brush, patch) from a given token stream.
 * The initial token, e.g. "brushDef3" is already parsed when the stream is passed to the
 * parse method.
 *
 * Such a class should not change its "state" during the parse() calls - the map parser
 * is calling the same instance of this PrimitiveParser over and over, one call for each
 * primitive, so when returning from the parse() method the class should be ready
 * to process the next primitive.
 */
class PrimitiveParser
{
public:
    virtual ~PrimitiveParser() {}

	/**
	 * Returns the primitive keyword of this parser, e.g. "brushDef3". When the Map parser
	 * encounters this keyword, the stream is passed along to the parse() method to create
	 * a scene node from it.
	 */
	virtual const std::string& getKeyword() const = 0;

	/**
	 * Creates and returns a primitive node according to the encountered token.
	 */
    virtual scene::INodePtr parse(parser::DefTokeniser& tok) const = 0;
};
typedef boost::shared_ptr<PrimitiveParser> PrimitiveParserPtr;

/**
 * Map Format interface. Each map format is able to traverse the scene graph and write
 * the contents into a mapfile, or to load a mapfile and populate a scene graph.
 */
class MapFormat :
	public RegisterableModule
{
public:
    virtual ~MapFormat() {}
	/**
	 * Read the contents of the given streams (which are contained in MapImportInfo)
	 * and add them as children to the given root node (also in MapImportInfo).
	 *
	 * @returns: TRUE on success, FALSE if parsing errors occurred.
	 */
	virtual bool readGraph(const MapImportInfo& importInfo) const = 0;

	/** Traverse the scene graph and write contents into the provided output stream.
	 *
	 * @param exportInfo
	 * The MapExportInfo structure, which contains the ostream references and such.
	 */
	virtual void writeGraph(const MapExportInfo& exportInfo) const = 0;
};
typedef boost::shared_ptr<MapFormat> MapFormatPtr;

/**
 * greebo: This is the global map format manager. Use this class to
 * register any parsers.
 */
class IMapFormatManager :
	public RegisterableModule
{
public:
	/**
	 * Registers a primitive parser. The "primitive type" variable
	 * refers to the keyword encountered when parsing a map, like brushDef3.
	 */
	virtual void registerPrimitiveParser(const PrimitiveParserPtr& parser) = 0;

	/**
	 * Returns a primitive parser for the given keyword, returns NULL if none found.
	 */
	virtual PrimitiveParserPtr getPrimitiveParser(const std::string& keyword) = 0;
};
typedef boost::shared_ptr<IMapFormatManager> IMapFormatManagerPtr;

} // namespace map

const std::string MODULE_MAPFORMATMANAGER("MapFormatManager");

// Application-wide Accessor to the global map format manager
inline map::IMapFormatManager& GlobalMapFormatManager()
{
	// Cache the reference locally
	static map::IMapFormatManager& _mapFormatManager(
		*boost::static_pointer_cast<map::IMapFormatManager>(
		module::GlobalModuleRegistry().getModule(MODULE_MAPFORMATMANAGER)
		)
	);
	return _mapFormatManager;
}

#endif /* _imapformat_h__ */

#pragma once 

#include "imodule.h"

namespace scene
{
class NodeVisitor;
class INode;
typedef boost::shared_ptr<INode> INodePtr;
}

namespace parser { class DefTokeniser; }

class Entity;
class IBrush;
class IPatch;

/** Callback function to control how the Walker traverses the scene graph. This function
 * will be provided to the map export module by the Radiant map code.
 */
typedef void (*GraphTraversalFunc) (scene::INodePtr root, scene::NodeVisitor& nodeExporter);

namespace map
{

// The MapExport/MapImport information structures, needed by the MapFormat::write() and
// MapFormat::read() methods. The actual definition of this structure is stored in the
// files libs/MapExportInfo.h and libs/MapImportInfo.h. TODO: DEPRECATED
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
 * An abstract map writer class used to write any map elements
 * as string to the given output stream. 
 *
 * The IMapWriter interface defines beginWrite/endWrite pairs for 
 * each scene element (Entity, primitives and the Map itself). 
 * These are called by the map saving algorithm when traversing 
 * the scene-depth-first. The usual call order will look like this:
 *
 * beginWriteMap
 *    beginWriteEntity
 *        beginWriteBrush
 *        endWriteBrush
 *        beginWritePatch
 *        endWritePatch
 *        ....
 *    endWriteEntity
 *    ...
 * endWriteMap
 *
 * Failure Handling: when the IMapWriter implementation encounters
 * errors during write (e.g. a visited node is not exportable) a
 * IMapWriter::FailureException will be thrown. The calling code
 * is designed to catch this exception.
 */
class IMapWriter 
{
public:
	// The generic exception type which is to be thrown by the 
	// IMapWriter methods
	class FailureException :
		public std::runtime_error
	{
	public:
		FailureException(const std::string& what) :
			std::runtime_error(what)
		{}
	};

	// Destructor
	virtual ~IMapWriter() {}
	
	/**
	 * This is called before writing any nodes, to give an opportunity
	 * to write a map header and version info.
	 */
	virtual void beginWriteMap(std::ostream& stream) = 0;

	/**
	 * Called after all nodes have been visited. Note that this method
	 * should NOT attempt to close the given stream.
	 */
	virtual void endWriteMap(std::ostream& stream) = 0;

	// Entity export methods
	virtual void beginWriteEntity(const Entity& entity, std::ostream& stream) = 0;
	virtual void endWriteEntity(const Entity& entity, std::ostream& stream) = 0;

	// Brush export methods
	virtual void beginWriteBrush(const IBrush& brush, std::ostream& stream) = 0;
	virtual void endWriteBrush(const IBrush& brush, std::ostream& stream) = 0;

	// Patch export methods
	virtual void beginWritePatch(const IPatch& patch, std::ostream& stream) = 0;
	virtual void endWritePatch(const IPatch& patch, std::ostream& stream) = 0;
};
typedef boost::shared_ptr<IMapWriter> IMapWriterPtr;

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
	 * Acquire a map writer instance, for exporting nodes to a stream.
	 */
	virtual IMapWriterPtr getMapWriter() const = 0;

	/**
	 * Read the contents of the given streams (which are contained in MapImportInfo)
	 * and add them as children to the given root node (also in MapImportInfo).
	 *
	 * @returns: TRUE on success, FALSE if parsing errors occurred.
	 * @DEPRECATED
	 */
	virtual bool readGraph(const MapImportInfo& importInfo) const = 0;

	/** Traverse the scene graph and write contents into the provided output stream.
	 *
	 * @param exportInfo
	 * The MapExportInfo structure, which contains the ostream references and such.
	 * @DEPRECATED
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

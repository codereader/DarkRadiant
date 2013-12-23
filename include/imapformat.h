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
typedef void (*GraphTraversalFunc) (const scene::INodePtr& root, scene::NodeVisitor& nodeExporter);

namespace map
{

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
 * the scene depth-first. The usual call order will look like this:
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
	// The generic exception type which is thrown by the IMapWriter methods
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
 * An abstract map reader class used to parse map elements
 * from the given input (string) stream. The map reader instance
 * is usually associated with an MapImportFilter class where the
 * parsed elements are sent to.
 */
class IMapReader
{
public:
	// The generic exception type which is thrown by a map reader
	class FailureException :
	public std::runtime_error
	{
	public:
		FailureException(const std::string& what) :
			std::runtime_error(what)
		{}
	};

	 /**
	 * Read the contents of the given stream and send them through the given MapImportFilter.
	 * Whether the nodes are actually added to the map or not is something the 
	 * ImportFilter can decide.
	 *
	 * throws: FailureException on any error.
	 */
	virtual void readFromStream(std::istream& stream) = 0;
};

class IMapImportFilter
{
public:
	/**
	 * Send an entity node to the import filter. In idTech4 maps all entities
	 * are immediate children of the root node in the scene, so this is where
     * they usually end up after being added (unless they're filtered out).
	 *
	 * @returns: true if the entity got added, false otherwise.
	 */
	virtual bool addEntity(const scene::INodePtr& entity) = 0;

	/**
	 * Add an primitive node to the given entity.
	 *
	 * @returns: true if the primitive got added, false otherwise.
	 */
	virtual bool addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity) = 0;
};
typedef boost::shared_ptr<IMapReader> IMapReaderPtr;

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
	 * Get the display name of this map format, e.g. "Doom 3", "Quake 4", etc.
	 */
	virtual const std::string& getMapFormatName() const = 0;

	/**
	 * Each MapFormat can have a certain game type it is designed for,
	 * a value which conincides with the type attribute in the game tag
	 * found in the .game file, e.g. "doom3" or "quake4".
	 */
	virtual const std::string& getGameType() const = 0;

	/**
	 * Instantiate a new map reader, using the given ImportFilter 
	 * which will be fed with nodes during the import.
	 */
	virtual IMapReaderPtr getMapReader(IMapImportFilter& filter) const = 0; 

	/**
	 * Acquire a map writer instance, for exporting nodes to a stream.
	 */
	virtual IMapWriterPtr getMapWriter() const = 0;

	/**
	 * Returns true if this map format allows the .darkradiant file
     * to be saved along the actual .map file. Some exporter modules
	 * might want to disable that (i.e. the prefab exporter)
	 */
	virtual bool allowInfoFileCreation() const = 0;

	/**
	 * greebo: Returns true if this map format is able to load
	 * the contents of this file. Usually this includes a version
	 * check of the file header.
	 */
	virtual bool canLoad(std::istream& stream) const = 0;
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
	 * Each MapFormat module should register itself here on startup.
	 */
	virtual void registerMapFormat(const std::string& extension, const MapFormatPtr& mapFormat) = 0;

	/**
	 * Proper MapFormat modules should unregister themselves at shutdown. This includes
	 * removal from all mapped extensions if the format was registered multiple times.
	 */
	virtual void unregisterMapFormat(const MapFormatPtr& mapFormat) = 0;

	/**
	 * Tries to look up the default map format for the given map format name.
	 */
	virtual MapFormatPtr getMapFormatByName(const std::string& mapFormatName) = 0;

	/**
	 * Tries to look up the default map format for the given game type (e.g. "doom3")
	 * associated with the given file extension.
	 */
	virtual MapFormatPtr getMapFormatForGameType(const std::string& gameType, 
												 const std::string& extension) = 0;

	/**
	 * Returns the list of registered map formats.
	 */
	virtual std::set<MapFormatPtr> getMapFormatList(const std::string& extension) = 0;
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

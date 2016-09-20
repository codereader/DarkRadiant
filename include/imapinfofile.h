#pragma once

#include <string>
#include <map>
#include "imodule.h"
#include "imap.h"
#include <functional>

namespace parser { class DefTokeniser; }
namespace scene { class INode; typedef std::shared_ptr<INode> INodePtr; }

namespace map
{

typedef std::pair<std::size_t, std::size_t> NodeIndexPair;
typedef std::map<NodeIndexPair, scene::INodePtr> NodeIndexMap;

/**
 * An info file module is allowed to write text-based information
 * to the auxiliary .darkradiant file that is written alongside to the 
 * game-compatible .map file. Things like layer or selection set/group
 * information can be stored persistently between mapping sessions this way.
 *
 * The module should write its information in named blocks, like 
 *
 * MyModuleInfo
 * {
 *      // arbitrary parseable info here
 * }
 *
 * Later, when the info file is parsed after map load, the module will be asked 
 * to parse the blocks it's responsible for, and apply its information to the map.
 */
class IMapInfoFileModule
{
public:
	virtual ~IMapInfoFileModule() {}

	// Info File Saving / Exporting

	// The name of this info file module, mainly for internal book-keeping
	virtual std::string getName() = 0;

	/**
	 * Called before any node is written to the .map file. Use tihs
	 * to prepare the internal structures for exporting.
	 */
	virtual void onInfoFileSaveStart() = 0;

	/**
	 * Called during map export traversal when a single
	 * primitive is about to be written to the .map file.
	 * Assemble information about the primitive and save it
	 * internally, until the writeBlocks() method is called.
	 */
	virtual void onSavePrimitive(const scene::INodePtr& node, 
		std::size_t entityNum, std::size_t primitiveNum) = 0;

	/**
	* Called during map export traversal when a single
	* entity is about to be written to the .map file.
	* Assemble information about the entity and save it
	* internally, until the writeBlocks() method is called.
	*/
	virtual void onSaveEntity(const scene::INodePtr& node, std::size_t entityNum) = 0;

	/**
	 * Final export function, write the assembled data to the
	 * info file stream. This method should include the block file name
	 * and the opening and closing braces in its write operation.
	 */
	virtual void writeBlocks(std::ostream& stream) = 0;

	/**
	 * Called before the info file stream is closed, time for cleanup.
	 */
	virtual void onInfoFileSaveFinished() = 0;

	// Info File Loading / Parsing

	/**
	 * Called before the info file is loaded, so take tihs opportunity to 
	 * clear internal structures that are going to be filled during the parse process.
	 */
	virtual void onInfoFileLoadStart() = 0;

	/**
	 * The info file parser will ask this module when a named block is encountered.
	 * A module which returns true on a block will sign up for parsing the block
	 * and a subsequent call to parseBlock() is imminent.
	 */
	virtual bool canParseBlock(const std::string& blockName) = 0;

	/**
	 * Parse a block as found in the info file. The block name as passed to this method
	 * needs to be registered in the IMapInfoFileManager class before.
	 *
	 * Regarding the state of the tokeniser: the block name will already have been parsed 
	 * by the time this method is called, so expect the opening brace { as first token.
	 */
	virtual void parseBlock(const std::string& blockName, parser::DefTokeniser& tok) = 0;

	/**
	 * Invoked by the map parsing code when the info file has been fully loaded,
	 * so modules should now apply the loaded information to the map.
	 * The info file is always loaded after the actual .map file, so this method
	 * can assume that the scene graph is already in place.
	 * For convenience, a NodeMap is passed to this method, mapping
	 * the entity/primitive number combination to scene::INodes.
	 */
	virtual void applyInfoToScene(const scene::IMapRootNodePtr& root, const NodeIndexMap& nodeMap) = 0;

	/**
	 * Post-parsing cleanup routine, called after applyInfoToScene().
	 */
	virtual void onInfoFileLoadFinished() = 0;
};
typedef std::shared_ptr<IMapInfoFileModule> IMapInfoFileModulePtr;

class IMapInfoFileManager :
	public RegisterableModule
{
public:
	virtual ~IMapInfoFileManager() 
	{}

	/**
	 * Add an info file module to the global list. The module will be considered
	 * during info file export/import.
	 */
	virtual void registerInfoFileModule(const IMapInfoFileModulePtr& module) = 0;

	/**
	 * Unregister a previouly registered info file module.
	 */
	virtual void unregisterInfoFileModule(const IMapInfoFileModulePtr& module) = 0;

	/**
	 * Call the functor for each registered module.
	 */
	virtual void foreachModule(const std::function<void(IMapInfoFileModule&)>& functor) = 0;
};

}

const char* const MODULE_MAPINFOFILEMANAGER("MapInfoFileManager");

// Application-wide Accessor to the global map info file manager
inline map::IMapInfoFileManager& GlobalMapInfoFileManager()
{
	// Cache the reference locally
	static map::IMapInfoFileManager& _manager(
		*std::static_pointer_cast<map::IMapInfoFileManager>(
			module::GlobalModuleRegistry().getModule(MODULE_MAPINFOFILEMANAGER))
	);
	return _manager;
}

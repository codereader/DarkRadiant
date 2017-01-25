#pragma once

#include "imodule.h"
#include "inode.h"
#include <sigc++/signal.h>

// Registry setting for suppressing the map load progress dialog
const char* const RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG = "user/ui/map/suppressMapLoadDialog";

// Namespace forward declaration
class INamespace;
typedef std::shared_ptr<INamespace> INamespacePtr;

// see mapfile.h
class IMapFileChangeTracker;

// see ientity.h
class ITargetManager;

namespace scene
{

/**
 * greebo: A root node is the top level element of a map.
 * It also owns the namespace of the corresponding map.
 */
class IMapRootNode :
    public virtual INode
{
public:
    virtual ~IMapRootNode() {}

    /**
     * greebo: Returns the namespace of this root.
     */
    virtual const INamespacePtr& getNamespace() = 0;

    /**
     * Returns the target manager keeping track of all
     * the named targets in the map.
     */
    virtual ITargetManager& getTargetManager() = 0;

    /**
     * The map root node is holding an implementation of the change tracker
     * interface, to keep track of whether the map resource on disk is
     * up to date or not.
     */
    virtual IMapFileChangeTracker& getUndoChangeTracker() = 0;
};
typedef std::shared_ptr<IMapRootNode> IMapRootNodePtr;

} // namespace scene

/**
 * greebo: This is the global interface to the currently
 * active map file.
 */
class IMap :
	public RegisterableModule
{
public:
	enum MapEvent
	{
		MapLoading,     // emitted just before a map is starting to be loaded
		MapLoaded,      // emitted when the current map is done loading
		MapUnloading,   // emitted just before a map is unloaded from memory
		MapUnloaded,    // emitted after a map has been unloaded
		MapSaving,		// emitted before a map is about to be saved (changes are possible)
		MapSaved,		// emitted right after a map has been saved
	};

	typedef sigc::signal<void, MapEvent> MapEventSignal;

	/// Returns the signal that is emitted on various events
	virtual MapEventSignal signal_mapEvent() const = 0;

	/**
	 * Returns the worldspawn node of this map. The worldspawn
	 * node is NOT created if it doesn't exist yet, so this
	 * might return an empty pointer.
	 */
	virtual const scene::INodePtr& getWorldspawn() = 0;

	/**
	 * This retrieves the worldspawn node of this map.
	 * If no worldspawn can be found, this creates one.
	 * Use this instead of getWorldSpawn() if your code needs
	 * a worldspawn to work with.
	 */
	virtual const scene::INodePtr& findOrInsertWorldspawn() = 0;

	/**
	 * Returns the root node of this map or NULL if this is an empty map.
	 */
	virtual scene::IMapRootNodePtr getRoot() = 0;

	/**
	* Returns the name of the map.
	*/
	virtual std::string getMapName() const = 0;
};
typedef std::shared_ptr<IMap> IMapPtr;

const std::string MODULE_MAP("Map");

// Application-wide Accessor to the currently active map
inline IMap& GlobalMapModule() {
	// Cache the reference locally
	static IMap& _mapModule(
		*std::static_pointer_cast<IMap>(
			module::GlobalModuleRegistry().getModule(MODULE_MAP)
		)
	);
	return _mapModule;
}

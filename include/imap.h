#pragma once

#include "imodule.h"
#include "inode.h"

// Registry setting for suppressing the map load progress dialog
const char* const RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG = "user/ui/map/suppressMapLoadDialog";

// Namespace forward declaration
class INamespace;
typedef std::shared_ptr<INamespace> INamespacePtr;

// see mapfile.h
class IMapFileChangeTracker;

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
	/**
	 * Returns the worldspawn node of this map. The worldspawn
	 * node is NOT created if it doesn't exist yet, so this
	 * might return an empty pointer.
	 */
	virtual scene::INodePtr getWorldspawn() = 0;

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

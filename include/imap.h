#pragma once

#include "imodule.h"
#include "inode.h"
#include "imapexporter.h"
#include "imapformat.h"
#include "imapmerge.h"
#include "ikeyvaluestore.h"
#include <sigc++/signal.h>

#include <os/fs.h>

// Registry setting for suppressing the map load progress dialog
const char* const RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG = "user/ui/map/suppressMapLoadDialog";

// Whether to load the most recently used map on app startup
const char* const RKEY_LOAD_LAST_MAP = "user/ui/map/loadLastMap";

const char* const LOAD_PREFAB_AT_CMD = "LoadPrefabAt";

// Namespace forward declaration
class INamespace;
typedef std::shared_ptr<INamespace> INamespacePtr;

// see imapfilechangetracker.h
class IMapFileChangeTracker;

// see ientity.h
class ITargetManager;

// see ilayer.h
class ILayerManager;

// see iundo.h
class IUndoSystem;

namespace selection
{
	class ISelectionSetManager;
	class ISelectionGroupManager;
}

namespace scene
{

/**
 * greebo: A root node is the top level element of a map.
 * It also owns the namespace of the corresponding map.
 */
class IMapRootNode :
    public virtual INode,
	public virtual IKeyValueStore
{
public:
    virtual ~IMapRootNode() {}

	virtual void setName(const std::string& name) = 0;

    /**
     * greebo: Returns the namespace of this root.
     */
    virtual const INamespacePtr& getNamespace() = 0;

	/**
	 * Access the selection group manager of this hierarchy.
	 */
	virtual selection::ISelectionGroupManager& getSelectionGroupManager() = 0;

	/**
	 * Gives access to the selectionset manager in this scene.
	 */
	virtual selection::ISelectionSetManager& getSelectionSetManager() = 0;

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

	/**
	 * Provides methods to create and assign layers in this map.
	 */
	virtual ILayerManager& getLayerManager() = 0;

    // The UndoSystem of this map
    virtual IUndoSystem& getUndoSystem() = 0;

    // Returns the render system of this map root (may be empty)
    virtual RenderSystemPtr getRenderSystem() const = 0;
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
		MapMergeOperationStarted,		// emitted after a merge operation has been started
		MapMergeOperationAborted,		// emitted after a merge operation has been aborted
		MapMergeOperationFinished,		// emitted after a merge operation has been finished
	};

	typedef sigc::signal<void, MapEvent> MapEventSignal;

	/// Returns the signal that is emitted on various events
	virtual MapEventSignal signal_mapEvent() const = 0;

    enum class EditMode
    {
        Normal,
        Merge,
    };

    // The currently active edit mode
    virtual EditMode getEditMode() = 0;

    // Change the edit mode to the specified value
    virtual void setEditMode(EditMode mode) = 0;

    // Signal fired when the map edit mode has been changed
    virtual sigc::signal<void, EditMode>& signal_editModeChanged() = 0;

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

    // Returns true if this map is still unnamed (never saved yet)
    virtual bool isUnnamed() const = 0;

	/**
	 * Signal fired when the name of this map is changing.
	 */
	virtual sigc::signal<void>& signal_mapNameChanged() = 0;

	// Returns true if the map has unsaved changes.
	virtual bool isModified() const = 0;

	// Sets the modified status of this map
	virtual void setModified(bool modifiedFlag) = 0;

	/**
	 * Signal fired when the modified status of this map has changed.
	 */
	virtual sigc::signal<void>& signal_modifiedChanged() = 0;

    // Signal emitted when an operation affecting the map has been undone
    virtual sigc::signal<void>& signal_postUndo() = 0;

    // Signal emitted when an operation affecting the map has been redone
    virtual sigc::signal<void>& signal_postRedo() = 0;

    // Accessor to the undo system of the main scene. 
    // Throws std::runtime_error if no map resource has been loaded.
    virtual IUndoSystem& getUndoSystem() = 0;

	// Caution: this is upposed to be called on startup, since it doesn't ask the user
	// whether to save the current map. Use the "NewMap" command for regular purposes.
	virtual void createNewMap() = 0;

	// Create a MapExporter instance which can be used to export a scene,
	// including the necessary preparation, info-file handling, etc.
	// This is mainly a service method for external code, like the gameconnection.
	virtual map::IMapExporter::Ptr createMapExporter(map::IMapWriter& writer,
		const scene::IMapRootNodePtr& root, std::ostream& mapStream) = 0;

    // Exports the current selection to the given output stream, using the map's format
    virtual void exportSelected(std::ostream& out) = 0;

    // Exports the current selection to the given output stream, using the given map format
    virtual void exportSelected(std::ostream& out, const map::MapFormatPtr& format) = 0;

    // Starts a merge operation which imports differences from the given sourceMap into this one
    // Will throw exceptions when the given map cannot be found, or this map doesn't have a root
    virtual void startMergeOperation(const std::string& sourceMap) = 0;

    // Starts a merge operation which imports the changes made to the source map into this one
    // baseMap defines the path to a map that both the source map and this map started from,
    // which makes the merge process more precise and enables conflict detection.
    // Will throw exceptions when the given map cannot be found, or this map doesn't have a root
    virtual void startMergeOperation(const std::string& sourceMap, const std::string& baseMap) = 0;

    // When called in EditMode::Merge, this will apply the currently active set of actions
    virtual void finishMergeOperation() = 0;

    // Can be called when in EditMode::Merge, will abort the current merge process
    virtual void abortMergeOperation() = 0;

    // Returns the currently active merge operation (or an empty reference if no merge is ongoing)
    virtual scene::merge::IMergeOperation::Ptr getActiveMergeOperation() = 0;

    /* POINTFILE MANAGEMENT */

    /// Functor to receive pointfile paths
    using PointfileFunctor = std::function<void(const fs::path&)>;

    /// Enumerate pointfiles associated with the current map
    virtual void forEachPointfile(PointfileFunctor func) const = 0;

    /**
     * \brief Show the point trace contained in the specified file.
     *
     * \param filePath
     * Filesystem path of the file to parse for point coordinates, or an empty
     * path to hide any current point trace.
     *
     * \exception std::runtime_error
     * Thrown if filePath is not empty but the file is inaccessible.
     */
    virtual void showPointFile(const fs::path& filePath) = 0;

    /// Return true if a point trace is currently visible
    virtual bool isPointTraceVisible() const = 0;
};
typedef std::shared_ptr<IMap> IMapPtr;

const char* const MODULE_MAP("Map");

// Application-wide Accessor to the currently active map
inline IMap& GlobalMapModule()
{
	static module::InstanceReference<IMap> _reference(MODULE_MAP);
	return _reference;
}


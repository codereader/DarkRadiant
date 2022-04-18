#pragma once

#include "inode.h"
#include "imodule.h"
#include "itextstream.h"
#include "imap.h"

#include <sigc++/signal.h>

namespace map 
{ 
	class MapFormat; 
	typedef std::shared_ptr<MapFormat> MapFormatPtr;
}

class IMapResource
{
public:
	virtual ~IMapResource() {}

	// Renames this map resource to the new path
	virtual void rename(const std::string& fullPath) = 0;

	/**
	 * Attempts to load the resource from disk. Returns true
	 * on success, in which case the getRootNode() method can be
	 * used to acquire a reference to the parsed map.
	 * Will throw an OperationException on failure.
	 */
	virtual bool load() = 0;

	// Exception type thrown by the the MapResource implementation
	class OperationException :
		public std::runtime_error 
	{
    private:
        bool _cancelled;

    public:
		OperationException(const std::string& msg) :
            OperationException(msg, false)
		{}

        OperationException(const std::string& msg, bool cancelled) :
            runtime_error(msg),
            _cancelled(cancelled)
        {
            if (!_cancelled)
            {
                rMessage() << "MapResource Operation: " << msg << std::endl;
            }
        }

        // Returns true if the operation has been cancelled by the user
        bool operationCancelled() const
        {
            return _cancelled;
        }
	};

    /**
     * Returns true if this resource refers to a read-only location.
     * This returns true for archived resources (loaded from PAK files),
     * but might be returning true for other resource types too (e.g. when
     * this process is lacking write access to the resource).
     */
    virtual bool isReadOnly() = 0;

	/**
	* Save this resource
	*
	* It's possible to pass a mapformat to be used for saving. If the map
	* format argument is omitted, the format corresponding to the current
	* game type is used.
	*
	* Throws an OperationException if anything prevented the save from
	* completion (user cancellation, I/O errors)
	*/
	virtual void save(const map::MapFormatPtr& mapFormat = map::MapFormatPtr()) = 0;

    virtual const scene::IMapRootNodePtr& getRootNode() = 0;

    // Sets the root node of this resource. The use case is to create a resource
    // around an existing map root for e.g. saving a read-only resource to a new path
    virtual void setRootNode(const scene::IMapRootNodePtr& root) = 0;

    virtual void clear() = 0;

    // Check if the target file on disk has been modified since it was last saved
    virtual bool fileOnDiskHasBeenModifiedSinceLastSave() = 0;

    // A signal that is emitted as soon as the map modified status changes due
    // to a regular edit, undo or redo. The bool value corresponds to the new modification state.
    virtual sigc::signal<void(bool)>& signal_modifiedStatusChanged() = 0;
};
typedef std::shared_ptr<IMapResource> IMapResourcePtr;

const char* const MODULE_MAPRESOURCEMANAGER("MapResourceManager");

class IMapResourceManager :
	public RegisterableModule
{
public:
	/**
	 * Create a named map resource from VFS or from a physical path.
	 */
	virtual IMapResourcePtr createFromPath(const std::string& path) = 0;

    /**
     * Create a named map resource that is contained within a PAK archive
     * outside the VFS.
     * archivePath is the absolute path to the archive file, e.g. "/home/greebo/outpost.pk4"
     * filePathWithinArchive is the relative path within the archive, e.g. "maps/outpost.map"
     */
    virtual IMapResourcePtr createFromArchiveFile(const std::string& archivePath, 
        const std::string& filePathWithinArchive) = 0;

	// Signal emitted when a MapExport is starting / is finished
	typedef sigc::signal<void, const scene::IMapRootNodePtr&> ExportEvent;

	// Event sent out right before a scene is sent to the exporter
	virtual ExportEvent& signal_onResourceExporting() = 0;

	// Event sent out right after a scene is sent to the exporter
	virtual ExportEvent& signal_onResourceExported() = 0;
};

inline IMapResourceManager& GlobalMapResourceManager()
{
	static module::InstanceReference<IMapResourceManager> _reference(MODULE_MAPRESOURCEMANAGER);
	return _reference;
}

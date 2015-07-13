#pragma once

#include <memory>
#include "imodule.h"

namespace map
{

/**
 * Representation of a Area Awareness System file.
 * Provides read-only access to Area and Portal information.
 * Use the GlobalAasFileManager() to acquire an instance of
 * this class.
 */
class IAasFile
{
public:
#if 0
    int							GetNumPlanes( void ) const { return planeList.Num(); }
	const idPlane &				GetPlane( int index ) const { return planeList[index]; }
	int							GetNumVertices( void ) const { return vertices.Num(); }
	const aasVertex_t &			GetVertex( int index ) const { return vertices[index]; }
	int							GetNumEdges( void ) const { return edges.Num(); }
	const aasEdge_t &			GetEdge( int index ) const { return edges[index]; }
	int							GetNumEdgeIndexes( void ) const { return edgeIndex.Num(); }
	const aasIndex_t &			GetEdgeIndex( int index ) const { return edgeIndex[index]; }
	int							GetNumFaces( void ) const { return faces.Num(); }
	const aasFace_t &			GetFace( int index ) const { return faces[index]; }
	int							GetNumFaceIndexes( void ) const { return faceIndex.Num(); }
	const aasIndex_t &			GetFaceIndex( int index ) const { return faceIndex[index]; }
	int							GetNumAreas( void ) const { return areas.Num(); }
	const aasArea_t &			GetArea( int index ) { return areas[index]; }
	int							GetNumNodes( void ) const { return nodes.Num(); }
	const aasNode_t &			GetNode( int index ) const { return nodes[index]; }
	int							GetNumPortals( void ) const { return portals.Num(); }
	const aasPortal_t &			GetPortal( int index ) { return portals[index]; }
	int							GetNumPortalIndexes( void ) const { return portalIndex.Num(); }
	const aasIndex_t &			GetPortalIndex( int index ) const { return portalIndex[index]; }
	int							GetNumClusters( void ) const { return clusters.Num(); }
	const aasCluster_t &		GetCluster( int index ) const { return clusters[index]; }
#endif
};
typedef std::shared_ptr<IAasFile> IAasFilePtr;

/**
 * A loader class capable of constructing an IAasFile instance from a token stream.
 */
class IAasFileLoader
{
public:
    /**
	 * Get the display name of this AAS file loader, e.g. "Doom 3", "Quake 4", etc.
	 */
	virtual const std::string& getAasFormatName() const = 0;

	/**
	 * Each MapFormat can have a certain game type it is designed for,
	 * a value which conincides with the type attribute in the game tag
	 * found in the .game file, e.g. "doom3" or "quake4".
	 */
	virtual const std::string& getGameType() const = 0;

	/**
	 * greebo: Returns true if this loader is able to parse
	 * the contents of this file. Usually this includes a version
	 * check of the file header.
	 */
	virtual bool canLoad(std::istream& stream) const = 0;

    /**
     * Load the AAS file contents from the given stream. 
     */
    virtual IAasFilePtr loadFromStream(std::istream& stream) = 0;
};
typedef std::shared_ptr<IAasFileLoader> IAasFileLoaderPtr;

class IAasFileManager : 
    public RegisterableModule
{
public:
    virtual ~IAasFileManager() {}

    // Register a loader which is considered by all future AAS file load attempts
    virtual void registerLoader(const IAasFileLoaderPtr& loader) = 0;

    // Unregister a previously registered loader instance
    virtual void unregisterLoader(const IAasFileLoaderPtr& loader) = 0;

    // Get a loader capable of loading the given stream
    virtual IAasFileLoaderPtr getLoaderForStream(std::istream& stream) = 0;
};

} // namespace

const char* const MODULE_AASFILEMANAGER("AasFileManager");

// Application-wide Accessor to the global AAS file manager
inline map::IAasFileManager& GlobalAasFileManager()
{
	// Cache the reference locally
	static map::IAasFileManager& _manager(
		*std::static_pointer_cast<map::IAasFileManager>(
		module::GlobalModuleRegistry().getModule(MODULE_AASFILEMANAGER))
	);
	return _manager;
}

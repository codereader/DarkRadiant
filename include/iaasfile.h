#pragma once

#include <memory>
#include "imodule.h"
#include <list>

#include "math/Plane3.h"
#include "math/Vector3.h"
#include "math/AABB.h"

namespace map
{

// An AAS type is defined by an entityDef block
// Each AAS type has its own file extension
struct AasType
{
    std::string entityDefName;
    std::string fileExtension;
};
typedef std::list<AasType> AasTypeList;

/**
 * Representation of a Area Awareness System file.
 * Provides read-only access to Area and Portal information.
 * Use the GlobalAasFileManager() to acquire an instance of
 * this class.
 */
class IAasFile
{
public:
    virtual std::size_t     getNumPlanes() const = 0;
    virtual const Plane3&   getPlane(std::size_t planeNum) const = 0;

    virtual std::size_t     getNumVertices() const = 0;
    virtual const Vector3&	getVertex(std::size_t vertexNum) const = 0;

    // An edge references two vertices by index
    struct Edge
    {
        int vertexNumber[2];
    };

    virtual std::size_t     getNumEdges() const = 0;
    virtual const Edge&     getEdge(std::size_t index) const = 0;

    virtual std::size_t     getNumEdgeIndexes() const = 0;
    virtual int 			getEdgeByIndex(int edgeIdx) const = 0;

    struct Face 
    {
	    int 				planeNum;   // number of the plane this face is on
	    unsigned short		flags;      // face flags
	    int					numEdges;   // number of edges in the boundary of the face
	    int					firstEdge;  // first edge in the edge index
	    short				areas[2];   // area at the front and back of this face
    };

    virtual std::size_t     getNumFaces() const = 0;
    virtual const Face&		getFace(int faceIndex) const = 0;
    virtual std::size_t     getNumFaceIndexes() const = 0;
    virtual int 			getFaceByIndex(int faceIdx) const = 0;

    struct Area
    {
        int             numFaces;			 // number of faces used for the boundary of the area
        int             firstFace;			 // first face in the face index used for the boundary of the area
        AABB            bounds;				 // bounds of the area
        Vector3         center;				 // center of the area an AI can move towards
        unsigned short  flags;				 // several area flags
        unsigned short  contents;			 // contents of the area
        short           cluster;			 // cluster the area belongs to, if negative it's a portal
        short           clusterAreaNum;		 // number of the area in the cluster
        int             travelFlags;		 // travel flags for traveling through this area
    };

    virtual std::size_t     getNumAreas() const = 0;
    virtual const Area&     getArea(int areaNum) const = 0;
};
typedef std::shared_ptr<IAasFile> IAasFilePtr;

/**
 * A loader class capable of constructing an IAasFile instance from a token stream.
 */
class IAasFileLoader :
    public RegisterableModule
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

// Info structure representing a single AAS file on disk
struct AasFileInfo
{
    std::string absolutePath;

    AasType type;
};

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

    // Get the list of valid AAS types
    virtual AasTypeList getAasTypes() = 0;

    // Returns a specific AAS type. Will throw a std::runtime_error if the 
    // type is not valid.
    virtual AasType getAasTypeByName(const std::string& typeName) = 0;

    // Returns a list of AAS files for the given map (absolute) map path
    virtual std::list<AasFileInfo> getAasFilesForMap(const std::string& mapPath) = 0;
};

} // namespace

const char* const MODULE_AASFILEMANAGER("ZAasFileManager");

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

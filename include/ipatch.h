#pragma once

#include "imodule.h"

#include <stdexcept>
#include "inode.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "render/VertexNT.h"

// This is thrown by the internal patch routines
class GenericPatchException :
	public std::runtime_error
{
public:
	// Constructor
	GenericPatchException(const std::string& what):
		std::runtime_error(what)
	{}
};

/* greebo: A PatchControl consists of a vertex and a set of texture coordinates.
 * Multiple PatchControls form a PatchControlArray or (together with width and height) a PatchControlMatrix.
 */
struct PatchControl
{
	Vector3 vertex;		// The coordinates of the control point
	Vector2 texcoord;	// The texture coordinates of this point
};

/**
 * A structure representing the fully tesselated patch
 * Can be acquired through the IPatch interface for
 * exporting the geometry to an external app.
 */
struct PatchMesh
{
	std::size_t width;	// width of this mesh
	std::size_t height; // height of this mesh

    /// Geometry with normals and texture coordinates
	std::vector<VertexNT> vertices;
};

typedef BasicVector2<unsigned int> Subdivisions;

// The abstract base class for a Doom3-compatible patch
class IPatch
{
public:
	// An observer can attach itself to a specific patch instance.
	// to get notified about changes.
	class Observer
	{
	public:
		/**
		 * Is called when the dimensions and/or the
		 * values of one or more control points get altered.
		 */
		virtual void onPatchControlPointsChanged() = 0;

		/**
		 * Is called when the patch shader is changed.
		 */
		virtual void onPatchTextureChanged() = 0;

		/**
		 * Is called by the Patch destructor. After this call
		 * the observer is automatically detached.
		 */
		virtual void onPatchDestruction() = 0;
	};

	virtual void attachObserver(Observer* observer) = 0;
	virtual void detachObserver(Observer* observer) = 0;

	virtual ~IPatch() {}

	// Resizes the patch to the given dimensions
	virtual void setDims(std::size_t width, std::size_t height) = 0;

	// Get the patch dimensions
	virtual std::size_t getWidth() const = 0;
	virtual std::size_t getHeight() const = 0;

	// Return a defined patch control vertex at <row>,<col>
	virtual PatchControl& ctrlAt(std::size_t row, std::size_t col) = 0;
	virtual const PatchControl& ctrlAt(std::size_t row, std::size_t col) const = 0;

	// Returns a copy of the fully tesselated patch geometry (slow!)
	virtual PatchMesh getTesselatedPatchMesh() const = 0;

	/**
	 * greebo: Inserts two columns before and after the column with index <colIndex>.
 	 * Throws an GenericPatchException if an error occurs.
 	 */
 	virtual void insertColumns(std::size_t colIndex) = 0;

 	/**
	 * greebo: Inserts two rows before and after the row with index <rowIndex>.
 	 * Throws an GenericPatchException if an error occurs.
 	 */
 	virtual void insertRows(std::size_t rowIndex) = 0;

	/**
	 * greebo: Removes columns or rows right before and after the col/row
 	 * with the given index, reducing the according dimension by 2.
 	 */
 	virtual void removePoints(bool columns, std::size_t index) = 0;

 	/**
	 * greebo: Appends two rows or columns at the beginning or the end.
 	 */
 	virtual void appendPoints(bool columns, bool beginning) = 0;

	// Updates the patch tesselation matrix, call this everytime you're done with your PatchControl changes
	virtual void controlPointsChanged() = 0;

	// Check if the patch has invalid control points or width/height are zero
	virtual bool isValid() const = 0;

	// Check whether all control vertices are in the same 3D spot (with minimal tolerance)
	virtual bool isDegenerate() const = 0;

	// Shader handling
	virtual const std::string& getShader() const = 0;
	virtual void setShader(const std::string& name) = 0;

	// greebo: returns true if the patch's shader is visible, false otherwise
	virtual bool hasVisibleMaterial() const = 0;

	/**
	 * greebo: Sets/gets whether this patch is a patchDef3 (fixed tesselation)
	 */
	virtual bool subdivisionsFixed() const = 0;

	/** greebo: Returns the x,y subdivision values (for tesselation)
	 */
	virtual const Subdivisions& getSubdivisions() const = 0;

	/** greebo: Sets the subdivision of this patch
	 *
	 * @isFixed: TRUE, if this patch should be a patchDef3 (fixed tesselation)
	 * @divisions: a two-component vector containing the desired subdivisions
	 */
	virtual void setFixedSubdivisions(bool isFixed, const Subdivisions& divisions) = 0;
};

namespace patch
{

enum class PatchEditVertexType : std::size_t
{
	Corners,
	Inside,
	NumberOfVertexTypes,
};

class IPatchSettings
{
public:
	virtual ~IPatchSettings() {}

	virtual const Vector3& getVertexColour(PatchEditVertexType type) const = 0;
	virtual void setVertexColour(PatchEditVertexType type, const Vector3& value) = 0;

	virtual sigc::signal<void>& signal_settingsChanged() = 0;
};

enum class PatchDefType
{
	Def2,
	Def3,
};

/**
 * Patch management module interface.
 */
class IPatchModule :
	public RegisterableModule
{
public:
	virtual ~IPatchModule() {}

	// Create a patch and return the sceneNode
	virtual scene::INodePtr createPatch(PatchDefType type) = 0;

	virtual IPatchSettings& getSettings() = 0;
};

}

class Patch;
class IPatchNode
{
public:
    virtual ~IPatchNode() {}

	/**
	 * greebo: Retrieves the actual patch from a PatchNode, only works from within the main module.
	 */
	virtual Patch& getPatchInternal() = 0;

	// Get access to the patch interface
	virtual IPatch& getPatch() = 0;
};
typedef std::shared_ptr<IPatchNode> IPatchNodePtr;

inline bool Node_isPatch(const scene::INodePtr& node)
{
    return node->getNodeType() == scene::INode::Type::Patch;
	//return std::dynamic_pointer_cast<IPatchNode>(node) != NULL;
}

inline IPatch* Node_getIPatch(const scene::INodePtr& node)
{
	auto patchNode = std::dynamic_pointer_cast<IPatchNode>(node);

	if (patchNode)
	{
		return &patchNode->getPatch();
	}

	return nullptr;
}

// Casts a node onto a patch
inline Patch* Node_getPatch(const scene::INodePtr& node)
{
	auto patchNode = std::dynamic_pointer_cast<IPatchNode>(node);

	if (patchNode)
	{
		return &patchNode->getPatchInternal();
	}

	return nullptr;
}

const char* const MODULE_PATCH = "PatchModule";

inline patch::IPatchModule& GlobalPatchModule()
{
	static patch::IPatchModule& _patchCreator(
		*std::static_pointer_cast<patch::IPatchModule>(
			module::GlobalModuleRegistry().getModule(MODULE_PATCH)
		)
	);

	return _patchCreator;
}

#pragma once

#include "inode.h"
#include "imodule.h"

#include "math/Vector2.h"
#include "math/Vector3.h"
#include <vector>

class Matrix4;
class Plane3;

const std::string RKEY_ENABLE_TEXTURE_LOCK("user/ui/brush/textureLock");

class BrushCreator :
	public RegisterableModule
{
public:
	virtual scene::INodePtr createBrush() = 0;
};

// The structure defining a single corner point of an IWinding
struct WindingVertex
{
	Vector3 vertex;			// The 3D coordinates of the point
	Vector2 texcoord;		// The UV coordinates
	Vector3 tangent;		// The tangent
	Vector3 bitangent;		// The bitangent
	Vector3 normal;			// The normals
	std::size_t adjacent;	// The index of the adjacent WindingVertex

	// greebo: This operator is needed to enable scripting support
	// using boost::python's vector_indexing_suite.
	bool operator==(const WindingVertex& other) const
	{
		return (vertex == other.vertex && texcoord == other.texcoord &&
			    tangent == other.tangent && bitangent == other.bitangent &&
				normal == other.normal && adjacent == other.adjacent);
	}
};

// A Winding consists of several connected WindingVertex objects,
// each of which holding information about a single corner point.
typedef std::vector<WindingVertex> IWinding;

// Interface for a face plane
class IFace
{
public:
	// Destructor
	virtual ~IFace() {}

	// Submits the current state to the UndoSystem, to make further actions undo-able
	virtual void undoSave() = 0;

	// Shader accessors
	virtual const std::string& getShader() const = 0;
	virtual void setShader(const std::string& name) = 0;

	// Shifts the texture by the given s,t amount in texture space
	virtual void shiftTexdef(float s, float t) = 0;

	// Scales the tex def by the given factors in texture space
	virtual void scaleTexdef(float s, float t) = 0;

	// Rotates the texture by the given angle
	virtual void rotateTexdef(float angle) = 0;

	// Fits the texture on this face
	virtual void fitTexture(float s_repeat, float t_repeat) = 0;

	// Flips the texture by the given flipAxis (0 == x-axis, 1 == y-axis)
	virtual void flipTexture(unsigned int flipAxis) = 0;

	// This translates the texture as much towards the origin in texture space as possible without changing the world appearance.
	virtual void normaliseTexture() = 0;

	// Get access to the actual Winding object
	virtual IWinding& getWinding() = 0;
	virtual const IWinding& getWinding() const = 0;

	virtual const Plane3& getPlane3() const = 0;

	/**
	 * Returns the 3x3 texture matrix for this face, containing shift, scale and rotation.
	 *
	 * xx, yx, xy and yy hold the scale and rotation
	 * tx and ty hold the shift
	 */
	virtual Matrix4 getTexDefMatrix() const = 0;
};

// Brush Interface
class IBrush
{
public:
    virtual ~IBrush() {}

	// Returns the number of faces for this brush
	virtual std::size_t getNumFaces() const = 0;

	// Get a reference to the face by index in [0..getNumFaces).
	virtual IFace& getFace(std::size_t index) = 0;

	// Const variant of the above
	virtual const IFace& getFace(std::size_t index) const = 0;

	// Add a new face to this brush, using the given plane object, returns a reference to the new face
	virtual IFace& addFace(const Plane3& plane) = 0;

	// Add a new face to this brush, using the given plane, texdef matrix and shader name
	virtual IFace& addFace(const Plane3& plane, const Matrix4& texDef, const std::string& shader) = 0;

	// Returns true when this brush has no faces
	virtual bool empty() const = 0;

	// Returns true if any face of the brush contributes to the final B-Rep.
	virtual bool hasContributingFaces() const = 0;

	// Removes faces that do not contribute to the brush.
	// This is useful for cleaning up after CSG operations on the brush.
	// Note: removal of empty faces is not performed during direct brush manipulations,
	// because it would make a manipulation irreversible if it created an empty face.
	virtual void removeEmptyFaces() = 0;

	// Sets the shader of all faces to the given name
	virtual void setShader(const std::string& newShader) = 0;

	// Returns TRUE if any of the faces has the given shader
	virtual bool hasShader(const std::string& name) = 0;

	// Returns TRUE if any of the brush's faces has a visible material, FALSE if all faces are effectively hidden
	virtual bool hasVisibleMaterial() const = 0;

	/**
	 * greebo: This is used by the filter system (for example) to trigger
	 * an update of the cached visibility flags. This enables a brush
	 * to quickly cull its hidden faces without issuing lots of internal calls.
	 */
	virtual void updateFaceVisibility() = 0;

	// Saves the current state to the undo stack.
	// Call this before manipulating the brush to make your action undo-able.
	virtual void undoSave() = 0;

	enum DetailFlag
	{
		Structural = 0,
		Detail = 1 << 27, // 134217728 
	};

	/**
	 * Q3-compatibility feature, get the value of the detail/structural flag
	 */
	virtual DetailFlag getDetailFlag() const = 0;

	/**
	 * Q3-compatibility feature, set the detail/structural flag
	 */
	virtual void setDetailFlag(DetailFlag newValue) = 0;
};

// Forward-declare the Brush object, only accessible from main binary
class Brush;

class IBrushNode
{
public:
    virtual ~IBrushNode() {}
	/** greebo: Retrieves the contained Brush from the BrushNode
	 */
	virtual Brush& getBrush() = 0;

	// Returns the IBrush interface
	virtual IBrush& getIBrush() = 0;
};
typedef boost::shared_ptr<IBrushNode> IBrushNodePtr;

inline bool Node_isBrush(const scene::INodePtr& node)
{
	return boost::dynamic_pointer_cast<IBrushNode>(node) != NULL;
}

// Casts the node onto a BrushNode and returns the Brush pointer
inline Brush* Node_getBrush(const scene::INodePtr& node)
{
	IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(node);
	if (brushNode != NULL) {
		return &brushNode->getBrush();
	}
	return NULL;
}

// Casts the node onto a BrushNode and returns the IBrush pointer
inline IBrush* Node_getIBrush(const scene::INodePtr& node)
{
	IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(node);
	if (brushNode != NULL) {
		return &brushNode->getIBrush();
	}
	return NULL;
}

const std::string MODULE_BRUSHCREATOR("Doom3BrushCreator");

inline BrushCreator& GlobalBrushCreator()
{
	// Cache the reference locally
	static BrushCreator& _brushCreator(
		*boost::static_pointer_cast<BrushCreator>(
			module::GlobalModuleRegistry().getModule(MODULE_BRUSHCREATOR)
		)
	);
	return _brushCreator;
}

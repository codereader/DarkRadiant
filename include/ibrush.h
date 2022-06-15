#pragma once

#include "inode.h"
#include "imodule.h"

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <vector>

class Plane3;

const std::string RKEY_ENABLE_TEXTURE_LOCK("user/ui/brush/textureLock");

namespace brush
{

// Helper class hosting brush-related settings
class IBrushSettings
{
public:
	virtual ~IBrushSettings() {}

	virtual const Vector3& getVertexColour() const = 0;
	virtual void setVertexColour(const Vector3& colour) = 0;

	virtual const Vector3& getSelectedVertexColour() const = 0;
	virtual void setSelectedVertexColour(const Vector3& colour) = 0;

	virtual sigc::signal<void>& signal_settingsChanged() = 0;
};

class BrushCreator :
	public RegisterableModule
{
public:
	virtual scene::INodePtr createBrush() = 0;

	virtual IBrushSettings& getSettings() = 0;
};

enum class PrefabType : int
{
	Cuboid = 0,
	Prism,
	Cone,
	Sphere,
	NumPrefabTypes,
};

// Public constants
const std::size_t c_brush_maxFaces = 1024;

const std::size_t PRISM_MIN_SIDES = 3;
const std::size_t PRISM_MAX_SIDES = c_brush_maxFaces - 2;

const std::size_t CONE_MIN_SIDES = 3;
const std::size_t CONE_MAX_SIDES = 32;

const std::size_t SPHERE_MIN_SIDES = 3;
const std::size_t SPHERE_MAX_SIDES = 7;

}

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

/**
 * greebo: The texture definition structure containing the scale,
 * rotation and shift values of an applied texture.
 * At some places this is referred to as "fake" texture coordinates.
 * This is not what is actually saved to the .map file, but it makes
 * texture manipulations in the Surface Inspector much more human-readable.
 */
struct ShiftScaleRotation
{
	double	shift[2];
	double	rotate;
	double	scale[2];

    ShiftScaleRotation()
    {
        shift[0] = shift[1] = 0;
        rotate = 0;
        scale[0] = scale[1] = 1;
    }
};

class IBrush;

// Interface for a face plane
class IFace
{
public:
	// Destructor
	virtual ~IFace() {}

    // Returns a reference to the brush containing this face
    virtual IBrush& getBrush() = 0;

	// Submits the current state to the UndoSystem, to make further actions undo-able
	virtual void undoSave() = 0;

	// Returns true if the texture of this face is not filtered out
	// This doesn't take into account whether the owning brush is visible or not
	virtual bool isVisible() const = 0;

	// Shader accessors
	virtual const std::string& getShader() const = 0;
	virtual void setShader(const std::string& name) = 0;

	// Shifts the texture by the given s,t amount in texture space
	virtual void shiftTexdef(float s, float t) = 0;

	// Convenience wrapper to shift the assigned texture by the given amount of pixels
	// the passed values are scaled accordingly and passed on to shiftTexdef()
	virtual void shiftTexdefByPixels(float s, float t) = 0;

	// Scales the tex def by the given factors in texture space
	virtual void scaleTexdef(float s, float t) = 0;

	// Rotates the texture by the given angle
	virtual void rotateTexdef(float angle) = 0;

    // Returns the amount of texture pixels per game unit shown on this face
    // This is based on the image returned by the material, usually the editor image
    virtual Vector2 getTexelScale() const = 0;

    // Returns the texture aspect ratio width/height
    virtual float getTextureAspectRatio() const = 0;

	// Fits the texture on this face
	virtual void fitTexture(float s_repeat, float t_repeat) = 0;

	// Flips the texture by the given flipAxis (0 == x-axis, 1 == y-axis)
	virtual void flipTexture(unsigned int flipAxis) = 0;

	// This translates the texture as much towards the origin in texture space as possible without changing the world appearance.
	virtual void normaliseTexture() = 0;

	enum class AlignEdge
	{
		Top,
		Bottom,
		Left,
		Right,
	};

	// If possible, aligns the assigned texture at the given anchor edge
	virtual void alignTexture(AlignEdge alignType) = 0;

    // Reverts any transform that has been applied since the last time freezeTransform() was called
    virtual void revertTransform() = 0;

    // Promotes the current transformed state to the new base state
    virtual void freezeTransform() = 0;

	// Get access to the actual Winding object
	virtual IWinding& getWinding() = 0;
	virtual const IWinding& getWinding() const = 0;

	virtual const Plane3& getPlane3() const = 0;

	/**
	 * The matrix used to project world coordinates to U/V space, after the winding vertices
     * have been transformed to this face's axis base system.
     * The components of this matrix correspond to the matrix values written to the idTech4 
     * brushDef3 face definition (with <zx, zy> holding the translation part):
     * e.g. ( plane ) ( ( xx yx zx ) ( yx yy zy ) ) "textures/path/to/material" 0 0 0
	 */
	virtual Matrix3 getProjectionMatrix() const = 0;

	virtual void setProjectionMatrix(const Matrix3& projection) = 0;

    // Constructs the texture projection matrix from the given (world) vertex and texture coords.
    // Three vertices and their UV coordinates are enough to construct the texdef.
    virtual void setTexDefFromPoints(const Vector3 points[3], const Vector2 uvs[3]) = 0;

	/**
	 * Calculates and returns the texture definition as shift/scale/rotate.
	 * This is not what is actually saved to the .map file, but it makes
	 * texture manipulations in the Surface Inspector much more human-readable.
	 */
	virtual ShiftScaleRotation getShiftScaleRotation() const = 0;
	virtual void setShiftScaleRotation(const ShiftScaleRotation& scr) = 0;

    // Transforms this face plane with the given transformation matrix
    virtual void transform(const Matrix4& transformation) = 0;

    // Emitted from this IFace's destructor, as last sign of life
    virtual sigc::signal<void>& signal_faceDestroyed() = 0;
};

// Plane classification info used by splitting and CSG algorithms
struct BrushSplitType 
{
	std::size_t counts[3];

	BrushSplitType()
	{
		counts[0] = 0;
		counts[1] = 0;
		counts[2] = 0;
	}

	BrushSplitType& operator+=(const BrushSplitType& other)
	{
		counts[0] += other.counts[0];
		counts[1] += other.counts[1];
		counts[2] += other.counts[2];
		return *this;
	}
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

	// Add a new face to this brush, using the given plane, projection matrix and material name
	virtual IFace& addFace(const Plane3& plane, const Matrix3& textureProjection, const std::string& material) = 0;

    // Removes all faces from this brush
    virtual void clear() = 0;

	// Returns true when this brush has no faces
	virtual bool empty() const = 0;

	// Returns true if any face of the brush contributes to the final B-Rep.
	virtual bool hasContributingFaces() const = 0;

    // Remove any faces from this brush that are not contributing anything to the resulting polyehdron
    // These are planes that have the same normal as an existing face and are superceded by them
    // This method is meant to be used during map loading to remove redundancy parsed from legacy maps
    virtual void removeRedundantFaces() = 0;

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

	// Classify this brush against the given plane, used by clipper and CSG algorithms
	virtual BrushSplitType classifyPlane(const Plane3& plane) const = 0;

	// Method used internally to recalculate the brush windings
	virtual void evaluateBRep() const = 0;
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
typedef std::shared_ptr<IBrushNode> IBrushNodePtr;

inline bool Node_isBrush(const scene::INodePtr& node)
{
    return node->getNodeType() == scene::INode::Type::Brush;
	//return std::dynamic_pointer_cast<IBrushNode>(node) != NULL;
}

// Casts the node onto a BrushNode and returns the Brush pointer
inline Brush* Node_getBrush(const scene::INodePtr& node)
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(node);
	if (brushNode != NULL) {
		return &brushNode->getBrush();
	}
	return NULL;
}

// Casts the node onto a BrushNode and returns the IBrush pointer
inline IBrush* Node_getIBrush(const scene::INodePtr& node)
{
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(node);
	if (brushNode != NULL) {
		return &brushNode->getIBrush();
	}
	return NULL;
}

const char* const MODULE_BRUSHCREATOR("Doom3BrushCreator");

inline brush::BrushCreator& GlobalBrushCreator()
{
	static module::InstanceReference<brush::BrushCreator> _reference(MODULE_BRUSHCREATOR);
	return _reference;
}

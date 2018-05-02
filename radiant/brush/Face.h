#pragma once

#include "irender.h"
#include "iundo.h"
#include "mapfile.h"
#include "iselectiontest.h"
#include <sigc++/connection.h>

#include "math/Vector3.h"

#include "TextureProjection.h"
#include "SurfaceShader.h"
#include "PlanePoints.h"
#include "FacePlane.h"
#include <memory>
#include "util/Noncopyable.h"
#include <sigc++/signal.h>
#include "selection/algorithm/Shader.h"

const double GRID_MIN = 0.125;

class Face;
typedef std::shared_ptr<Face> FacePtr;
typedef std::vector<FacePtr> Faces;

/// A single planar face of a brush
class Face :
	public IFace,
	public IUndoable,
	public util::Noncopyable
{
private:
    // The structure which is saved to the undo stack
    class SavedState;

public:
	PlanePoints m_move_planepts;
	PlanePoints m_move_planeptsTransformed;

private:
	// The parent brush
	Brush& _owner;

	FacePlane m_plane;
	FacePlane m_planeTransformed;

    // Face shader, stores material name and GL shader object
	SurfaceShader _shader;

	// Connected to the SurfaceShader signals
	sigc::connection _surfaceShaderRealised;

	TextureProjection _texdef;
	TextureProjection m_texdefTransformed;

	Winding m_winding;
	Vector3 m_centroid;

	IUndoStateSaver* _undoStateSaver;

	// Cached visibility flag, queried during front end rendering
	bool _faceIsVisible;

public:

	// Constructors
	Face(Brush& owner);
	Face(Brush& owner, const Vector3& p0, const Vector3& p1, const Vector3& p2,
		const std::string& shader, const TextureProjection& projection);

	Face(Brush& owner, const Plane3& plane);
	Face(Brush& owner, const Plane3& plane, const Matrix4& texdef,
		 const std::string& shader);

	// Copy Constructor
	Face(Brush& owner, const Face& other);

	// Destructor
	virtual ~Face();

	// Get the parent brush object
	Brush& getBrush();

	void planeChanged();

	// greebo: Emits the updated normals to the Winding class.
	void updateWinding();

    void connectUndoSystem(IMapFileChangeTracker& changeTracker);
    void disconnectUndoSystem(IMapFileChangeTracker& changeTracker);

	void undoSave();

	// undoable
	IUndoMementoPtr exportState() const;
	void importState(const IUndoMementoPtr& data);

    /// Translate the face by the given vector
    void translate(const Vector3& translation);

	void flipWinding();

	bool intersectVolume(const VolumeTest& volume) const;
	bool intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	// Frontend render methods for submitting the face winding
	void renderSolid(RenderableCollector& collector, const Matrix4& localToWorld,
		const IRenderEntity& entity, const LightList& lights) const;
	void renderWireframe(RenderableCollector& collector, const Matrix4& localToWorld,
		const IRenderEntity& entity) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void transform(const Matrix4& matrix);

	void assign_planepts(const PlanePoints planepts);

	/// \brief Reverts the transformable state of the brush to identity.
	void revertTransform();
	void freezeTransform();

	void update_move_planepts_vertex(std::size_t index, PlanePoints planePoints);

	void snapto(float snap);

	void testSelect(SelectionTest& test, SelectionIntersection& best);
	void testSelect_centroid(SelectionTest& test, SelectionIntersection& best);

	void shaderChanged();

	const std::string& getShader() const;
	void setShader(const std::string& name);

	void revertTexdef();
	void texdefChanged();

    const TextureProjection& getProjection() const;
    TextureProjection& getProjection();

	void GetTexdef(TextureProjection& projection) const;
	void SetTexdef(const TextureProjection& projection);

    // Applies the given shift/scale/rotation values to this face's texture projection
    // The incoming values are measured in pixels and will be scaled internally.
    void setTexdef(const TexDef& texDef);

	/**
	 * greebo: Copies the shader (texdef) from the other face,
	 * and attempts to move the texture such that the transition
	 * between the faces are seamless.
	 */
	void applyShaderFromFace(const Face& other);

    // s and t are texture coordinates
	void shiftTexdef(float s, float t);

    // Same as above, but with pixel values
    void shiftTexdefByPixels(float sPixels, float tPixels);

    // Scale the texdef by the given factors in s and t direction
    // Passing s=1.05 will scale the texture to 105% in the s dimension
	void scaleTexdef(float sFactor, float tFactor);
	void rotateTexdef(float angle);
	void fitTexture(float s_repeat, float t_repeat);
	void flipTexture(unsigned int flipAxis);
	void alignTexture(EAlignType align);

	/** greebo: This translates the texture as much towards
	 * 	the origin as possible. The face appearance stays unchanged.
	 */
	void normaliseTexture();

	void EmitTextureCoordinates();

    // When constructing faces with a default-constructed TextureProjection the scale is very small
    // fix that by calling this method.
    void applyDefaultTextureScale();

	const Vector3& centroid() const;

	void construct_centroid();

	const Winding& getWinding() const;
	Winding& getWinding();

	const Plane3& plane3() const;

	// Returns the Doom 3 plane
	const Plane3& getPlane3() const;

	FacePlane& getPlane();
	const FacePlane& getPlane() const;

	Matrix4 getTexDefMatrix() const;

	SurfaceShader& getFaceShader();
	const SurfaceShader& getFaceShader() const;

	bool contributes() const;
	bool is_bounded() const;

	bool faceIsVisible() const
	{
		return _faceIsVisible;
	}

	void updateFaceVisibility();

	// Signal for external code to get notified each time the texdef of any face changes
	static sigc::signal<void>& signal_texdefChanged();

private:
	void realiseShader();

	// Connects surface shader signals and calls realiseShader() if possible
	void setupSurfaceShader();

}; // class Face

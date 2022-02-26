#pragma once

#include "irender.h"
#include "iundo.h"
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
#include "RenderableWinding.h"

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

    render::RenderableWinding _windingSurfaceSolid;
    render::RenderableWinding _windingSurfaceWireframe;

    sigc::signal<void> _sigDestroyed;

public:

	// Constructors
	Face(Brush& owner);
	Face(Brush& owner, const Vector3& p0, const Vector3& p1, const Vector3& p2,
		const std::string& shader, const TextureProjection& projection);

	Face(Brush& owner, const Plane3& plane);
	Face(Brush& owner, const Plane3& plane, const Matrix3& textureProjection, const std::string& material);

	// Copy Constructor
	Face(Brush& owner, const Face& other);

	// Destructor
	virtual ~Face();

	// Get the parent brush object
	IBrush& getBrush() override;

    Brush& getBrushInternal();

    sigc::signal<void>& signal_faceDestroyed() override;

	void planeChanged();

	// greebo: Emits the updated normals to the Winding class.
	void updateWinding();

    void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

	void undoSave() override;

	// undoable
	IUndoMementoPtr exportState() const override;
	void importState(const IUndoMementoPtr& data) override;

    /// Translate the face by the given vector
    void translate(const Vector3& translation);

	void flipWinding();

	bool intersectVolume(const VolumeTest& volume) const;
	bool intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void transform(const Matrix4& matrix) override;

	void assign_planepts(const PlanePoints planepts);

	/// \brief Reverts the transformable state of the brush to identity.
	void revertTransform() override;
	void freezeTransform() override;

	void update_move_planepts_vertex(std::size_t index, PlanePoints planePoints);

	void snapto(float snap);

	void testSelect(SelectionTest& test, SelectionIntersection& best);
	void testSelect_centroid(SelectionTest& test, SelectionIntersection& best);

	void shaderChanged();

	const std::string& getShader() const override;
	void setShader(const std::string& name) override;

	void revertTexdef();
	void texdefChanged();

    const TextureProjection& getProjection() const;
    TextureProjection& getProjection();

	void GetTexdef(TextureProjection& projection) const;
	void SetTexdef(const TextureProjection& projection);

    // Constructs the texture projection matrix from the given (world) vertex and texture coords.
    // Three vertices and their UV coordinates are enough to construct the texdef.
    void setTexDefFromPoints(const Vector3 points[3], const Vector2 uvs[3]) override;

	ShiftScaleRotation getShiftScaleRotation() const override;
	void setShiftScaleRotation(const ShiftScaleRotation& ssr) override;

	/**
	 * greebo: Copies the shader (texdef) from the other face,
	 * and attempts to move the texture such that the transition
	 * between the faces are seamless.
	 */
	void applyShaderFromFace(const Face& other);

    // s and t are texture coordinates
	void shiftTexdef(float s, float t) override;

    // Same as above, but with pixel values
    void shiftTexdefByPixels(float sPixels, float tPixels) override;

    // Scale the texdef by the given factors in s and t direction
    // Passing s=1.05 will scale the texture to 105% in the s dimension
	void scaleTexdef(float sFactor, float tFactor) override;
	void rotateTexdef(float angle) override;

    Vector2 getTexelScale() const override;
    float getTextureAspectRatio() const override;

	void fitTexture(float s_repeat, float t_repeat) override;
	void flipTexture(unsigned int flipAxis) override;
	void alignTexture(AlignEdge alignType) override;

	/** greebo: This translates the texture as much towards
	 * 	the origin as possible. The face appearance stays unchanged.
	 */
	void normaliseTexture() override;

	void emitTextureCoordinates();

    // When constructing faces with a default-constructed TextureProjection the scale is very small
    // fix that by calling this method.
    void applyDefaultTextureScale();

	const Vector3& centroid() const;

	void construct_centroid();

	const Winding& getWinding() const override;
	Winding& getWinding() override;

    render::RenderableWinding& getWindingSurfaceSolid();
    render::RenderableWinding& getWindingSurfaceWireframe();

	const Plane3& plane3() const;

	// Returns the Doom 3 plane
	const Plane3& getPlane3() const override;

	FacePlane& getPlane();
	const FacePlane& getPlane() const;

	Matrix3 getProjectionMatrix() const override;
	void setProjectionMatrix(const Matrix3& projection) override;

	SurfaceShader& getFaceShader();
	const SurfaceShader& getFaceShader() const;

	bool contributes() const;
	bool is_bounded() const;

	bool isVisible() const override;

    // Called when the owning brush changes its visibility status
    void onBrushVisibilityChanged(bool visible);

	void updateFaceVisibility();

	// Signal for external code to get notified each time the texdef of any face changes
	static sigc::signal<void>& signal_texdefChanged();

private:
	void realiseShader();

	// Connects surface shader signals and calls realiseShader() if possible
	void setupSurfaceShader();

    // Transforms the texdef using the given world transform, with the goal
    // to keep the texture coordinates of the winding unaltered by the transform
    void transformTexDefLocked(const Matrix4& transform);

    void clearRenderables();
    void updateRenderables();
};

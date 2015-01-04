#pragma once

#include "irender.h"
#include "iundo.h"
#include "mapfile.h"
#include "iselectiontest.h"

#include "math/Vector3.h"

#include "FaceTexDef.h"
#include "SurfaceShader.h"
#include "PlanePoints.h"
#include "FacePlane.h"
#include <memory>
#include <boost/noncopyable.hpp>
#include "selection/algorithm/Shader.h"

const double GRID_MIN = 0.125;

class Face;
typedef std::shared_ptr<Face> FacePtr;
typedef std::vector<FacePtr> Faces;

class FaceObserver {
public:
    virtual ~FaceObserver() {}
	virtual void planeChanged() = 0;
	virtual void connectivityChanged() = 0;
	virtual void shaderChanged() = 0;
	virtual void evaluateTransform() = 0;
};

/// A single planar face of a brush
class Face :
	public IFace,
	public IUndoable,
	public SurfaceShader::Observer,
	public boost::noncopyable
{
	class SavedState : 
		public IUndoMemento
	{
		public:
			FacePlane::SavedState _planeState;
			FaceTexdef::SavedState _texdefState;
            std::string _materialName;

		SavedState(const Face& face) :
			_planeState(face.getPlane()),
			_texdefState(face.getTexdef()),
            _materialName(face.getShader())
		{}

		virtual ~SavedState() {}

		void exportState(Face& face) const {
			_planeState.exportState(face.getPlane());
            face.setShader(_materialName);
			_texdefState.exportState(face.getTexdef());
		}
	};

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

	FaceTexdef m_texdef;
	TextureProjection m_texdefTransformed;

	Winding m_winding;
	Vector3 m_centroid;

	FaceObserver* m_observer;
	IUndoStateSaver* _undoStateSaver;

	// Cached visibility flag, queried during front end rendering
	bool _faceIsVisible;

public:

	// Constructors
	Face(Brush& owner, FaceObserver* observer);
	Face(Brush& owner, const Vector3& p0, const Vector3& p1, const Vector3& p2,
		const std::string& shader, const TextureProjection& projection, FaceObserver* observer);

	Face(Brush& owner, const Plane3& plane, FaceObserver* observer);
	Face(Brush& owner, const Plane3& plane, const Matrix4& texdef,
		 const std::string& shader, FaceObserver* observer);

	// Copy Constructor
	Face(Brush& owner, const Face& other, FaceObserver* observer);

	// Destructor
	virtual ~Face();

	// Get the parent brush object
	Brush& getBrush();

	void planeChanged();

	// greebo: Emits the updated normals to the Winding class.
	void updateWinding();

	void realiseShader();
	void unrealiseShader();

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

    /**
     * \brief
     * Submit renderable geometry to a RenderableCollector.
     */
	void submitRenderables(RenderableCollector& collector,
                           const Matrix4& localToWorld,
						   const IRenderEntity& entity) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void transform(const Matrix4& matrix, bool mirror);

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

	void GetTexdef(TextureProjection& projection) const;
	void SetTexdef(const TextureProjection& projection);

	/**
	 * greebo: Copies the shader (texdef) from the other face,
	 * and attempts to move the texture such that the transition
	 * between the faces are seamless.
	 */
	void applyShaderFromFace(const Face& other);

	void shiftTexdef(float s, float t);
	void scaleTexdef(float s, float t);
	void rotateTexdef(float angle);
	void fitTexture(float s_repeat, float t_repeat);
	void flipTexture(unsigned int flipAxis);
	void alignTexture(EAlignType align);

	/** greebo: This translates the texture as much towards
	 * 	the origin as possible. The face appearance stays unchanged.
	 */
	void normaliseTexture();

	void EmitTextureCoordinates();

	const Vector3& centroid() const;

	void construct_centroid();

	const Winding& getWinding() const;
	Winding& getWinding();

	const Plane3& plane3() const;

	// Returns the Doom 3 plane
	const Plane3& getPlane3() const;

	FacePlane& getPlane();
	const FacePlane& getPlane() const;

	FaceTexdef& getTexdef();
	const FaceTexdef& getTexdef() const;
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

}; // class Face

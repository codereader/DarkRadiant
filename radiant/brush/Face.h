#ifndef FACE_H_
#define FACE_H_

#include "irender.h"
#include "iundo.h"
#include "mapfile.h"
#include "selectable.h"

#include "math/Vector3.h"
#include "math/matrix.h"

#include "FaceTexDef.h"
#include "FaceShader.h"
#include "PlanePoints.h"
#include "FacePlane.h"
#include <boost/shared_ptr.hpp>

const double GRID_MIN = 0.125;

typedef double (*QuantiseFunc)(double f);

class Face;
typedef boost::shared_ptr<Face> FacePtr;
typedef std::vector<FacePtr> Faces;

class FaceObserver {
public:
	virtual void planeChanged() = 0;
	virtual void connectivityChanged() = 0;
	virtual void shaderChanged() = 0;
	virtual void evaluateTransform() = 0;
};

class Face :
	public OpenGLRenderable,
	public Undoable,
	public FaceShaderObserver
{
	std::size_t m_refcount;

	class SavedState : public UndoMemento {
		public:
			FacePlane::SavedState m_planeState;
			FaceTexdef::SavedState m_texdefState;
			FaceShader::SavedState m_shaderState;

		SavedState(const Face& face) : m_planeState(face.getPlane()), m_texdefState(face.getTexdef()), m_shaderState(face.getShader())
		{}

		void exportState(Face& face) const {
			m_planeState.exportState(face.getPlane());
			m_shaderState.exportState(face.getShader());
			m_texdefState.exportState(face.getTexdef());
		}

		void release() {
			delete this;
		}
	};

public:
	static QuantiseFunc m_quantise;

	PlanePoints m_move_planepts;
	PlanePoints m_move_planeptsTransformed;
private:
	FacePlane m_plane;
	FacePlane m_planeTransformed;
	FaceShader m_shader;
	FaceTexdef m_texdef;
	TextureProjection m_texdefTransformed;
	
	Winding m_winding;
	Vector3 m_centroid;
	
	FaceObserver* m_observer;
	UndoObserver* m_undoable_observer;
	MapFile* m_map;
	
	// assignment not supported
	Face& operator=(const Face& other);
	// copy-construction not supported
	Face(const Face& other);

public:

	// Constructors
	Face(FaceObserver* observer);
	Face(const Vector3& p0, const Vector3& p1, const Vector3& p2,
		const std::string& shader, const TextureProjection& projection, FaceObserver* observer);
		
	// Copy Constructor
	Face(const Face& other, FaceObserver* observer);
	
	// Destructor
	~Face();

	void planeChanged();
	
	// greebo: Emits the updated normals to the Winding class.
	void updateWinding();
	
	void realiseShader();
	void unrealiseShader();
	
	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);
	
	void render(RenderStateFlags state) const;

	void undoSave();

	// undoable
	UndoMemento* exportState() const;
	void importState(const UndoMemento* data);

	void flipWinding();

	bool intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	void render(Renderer& renderer, const Matrix4& localToWorld) const;

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

	const std::string& GetShader() const;
	void SetShader(const std::string& name);

	void revertTexdef();
	void texdefChanged();

	void GetTexdef(TextureProjection& projection) const;
	void SetTexdef(const TextureProjection& projection);
	
	void GetFlags(ContentsFlagsValue& flags) const;
	void SetFlags(const ContentsFlagsValue& flags);
	
	void ShiftTexdef(float s, float t);
	void ScaleTexdef(float s, float t);
	void RotateTexdef(float angle);
	void FitTexture(float s_repeat, float t_repeat);
	void flipTexture(unsigned int flipAxis);
	
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
	
	FacePlane& getPlane();
	const FacePlane& getPlane() const;
	
	FaceTexdef& getTexdef();
	const FaceTexdef& getTexdef() const;
	FaceShader& getShader();
	const FaceShader& getShader() const;
	
	bool contributes() const;
	bool is_bounded() const;

}; // class Face

#endif /*FACE_H_*/

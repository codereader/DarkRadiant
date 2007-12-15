#ifndef FACETEXDEF_H_
#define FACETEXDEF_H_

#include "Winding.h"
#include "math/matrix.h"
#include "math/Vector3.h"
#include "math/Plane3.h"

#include "FaceShader.h"
#include "TextureProjection.h"

class FaceTexdef : public FaceShaderObserver {
	
	// not copyable
	FaceTexdef(const FaceTexdef& other);
	// not assignable
	FaceTexdef& operator=(const FaceTexdef& other);

public:
	class SavedState {
	public:
		TextureProjection m_projection;
	
		SavedState(const FaceTexdef& faceTexdef) {
			m_projection = faceTexdef.m_projection;
		}
	
		void exportState(FaceTexdef& faceTexdef) const {
			faceTexdef.m_projection.assign(m_projection);
		}
	};

	FaceShader& m_shader;
	TextureProjection m_projection;
	bool m_projectionInitialised;
	bool m_scaleApplied;

	// Constructor
	FaceTexdef(FaceShader& shader, const TextureProjection& projection, bool projectionInitialised = true);
	
	// Destructor
	~FaceTexdef();

	// Remove the scaling of the texture coordinates
	void addScale();
	void removeScale();

	void realiseShader();
	void unrealiseShader();
	
	void setTexdef(const TextureProjection& projection);
	
	void shift(float s, float t);
	void scale(float s, float t);
	void rotate(float angle);

	void fit(const Vector3& normal, const Winding& winding, float s_repeat, float t_repeat);

	// Mirrors the texture around the given axis
	void flipTexture(unsigned int flipAxis);

	// greebo: Calculate the texture coordinates and save them into the winding points
	void emitTextureCoordinates(Winding& winding, const Vector3& normal, const Matrix4& localToWorld);

	void transform(const Plane3& plane, const Matrix4& matrix);

	TextureProjection normalised() const;
	
	void setBasis(const Vector3& normal);

}; // class FaceTexDef

#endif /*FACETEXDEF_H_*/

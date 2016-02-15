#pragma once

#include "Winding.h"
#include "math/Vector3.h"
#include "math/Plane3.h"

#include "SurfaceShader.h"
#include "TextureProjection.h"
#include <boost/noncopyable.hpp>
#include "selection/algorithm/Shader.h"

class Matrix4;

class FaceTexdef :
	public SurfaceShader::Observer,
	public boost::noncopyable
{
public:
	class SavedState
	{
	public:
		TextureProjection m_projection;

		SavedState(const FaceTexdef& faceTexdef) {
			m_projection = faceTexdef.m_projection;
		}

		void exportState(FaceTexdef& faceTexdef) const {
			faceTexdef.m_projection.assign(m_projection);
		}
	};

    SurfaceShader& m_shader;
	TextureProjection m_projection;
	bool m_projectionInitialised;

private:
	bool m_scaleApplied;

public:
	// Constructor
    FaceTexdef(SurfaceShader& shader, const TextureProjection& projection, bool projectionInitialised = true);

	// Destructor
	virtual ~FaceTexdef();

	// Remove the scaling of the texture coordinates
	void addScale();
	void removeScale();
    // This is only used by the Face constructor invoked by the map loading code
    // texdefs loaded from the D3 .map format already have their scale applied.
    void setScaleApplied(bool applied);

	void realiseShader();
	void unrealiseShader();

	void setTexdef(const TextureProjection& projection);

    // s and t are texture coordinates, not pixel values
	void shift(float s, float t);

	void scale(float s, float t);
	void rotate(float angle);

	void fit(const Vector3& normal, const Winding& winding, float s_repeat, float t_repeat);

	// Mirrors the texture around the given axis
	void flipTexture(unsigned int flipAxis);

	// Aligns this texture to the given edge
	void alignTexture(EAlignType align, const Winding& winding);

	// greebo: Calculate the texture coordinates and save them into the winding points
	void emitTextureCoordinates(Winding& winding, const Vector3& normal, const Matrix4& localToWorld);
#if 0
	void transform(const Plane3& plane, const Matrix4& matrix);

	TextureProjection normalised() const;
#endif
	void setBasis(const Vector3& normal);

}; // class FaceTexDef

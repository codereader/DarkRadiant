#pragma once

#include "Winding.h"
#include "math/Vector3.h"
#include "math/Plane3.h"

#include "TextureProjection.h"
#include <boost/noncopyable.hpp>
#include "selection/algorithm/Shader.h"

class SurfaceShader;
class Matrix4;

class FaceTexdef :
	public boost::noncopyable
{
public:
	class SavedState
	{
	public:
		TextureProjection m_projection;

		SavedState(const FaceTexdef& faceTexdef) {
			m_projection = faceTexdef.getProjection();
		}

		void exportState(FaceTexdef& faceTexdef) const {
			faceTexdef.getProjection().assign(m_projection);
		}
	};

private:
    SurfaceShader& _shader;
	TextureProjection _projection;

public:
    // Constructor (default TextureProjection)
    FaceTexdef(SurfaceShader& shader);

	// Constructor (copying existing TextureProjection)
    FaceTexdef(SurfaceShader& shader, const TextureProjection& projection);

    const TextureProjection& getProjection() const;
    TextureProjection& getProjection();

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

    void setBasis(const Vector3& normal);

}; // class FaceTexDef

#include "FaceTexDef.h"

#include "SurfaceShader.h"

// Constructor
FaceTexdef::FaceTexdef(SurfaceShader& shader, const TextureProjection& projection) :
	m_shader(shader),
	m_projection(projection)
{}

void FaceTexdef::setTexdef(const TextureProjection& projection)
{
	m_projection.assign(projection);
}

void FaceTexdef::shift(float s, float t) 
{
	ASSERT_MESSAGE(m_projection.m_texdef.isSane(), "FaceTexdef::shift: bad texdef");
	m_projection.shift(s, t);
}

void FaceTexdef::scale(float s, float t)
{
	m_projection.scale(s, t, m_shader.getWidth(), m_shader.getHeight());
}

void FaceTexdef::rotate(float angle)
{
	m_projection.rotate(angle, m_shader.getWidth(), m_shader.getHeight());
}

void FaceTexdef::fit(const Vector3& normal, const Winding& winding, float s_repeat, float t_repeat) 
{
	m_projection.fitTexture(m_shader.getWidth(), m_shader.getHeight(), normal, winding, s_repeat, t_repeat);
}

void FaceTexdef::flipTexture(unsigned int flipAxis)
{
	m_projection.flipTexture(flipAxis);
}

void FaceTexdef::alignTexture(EAlignType align, const Winding& winding)
{
	m_projection.alignTexture(align, winding);
}

void FaceTexdef::emitTextureCoordinates(Winding& winding, const Vector3& normal, const Matrix4& localToWorld) 
{
	// old code // m_projection.emitTextureCoordinates(m_shader.width(), m_shader.height(), winding, normal, localToWorld);
	m_projection.emitTextureCoordinates(winding, normal, localToWorld);
}

void FaceTexdef::setBasis(const Vector3& normal)
{
	Matrix4 basis;
	Normal_GetTransform(normal, basis);
}

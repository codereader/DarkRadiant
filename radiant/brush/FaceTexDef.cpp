#include "FaceTexDef.h"

#include "SurfaceShader.h"

FaceTexdef::FaceTexdef(SurfaceShader& shader) :
    FaceTexdef(shader, TextureProjection()) // default projection
{}

FaceTexdef::FaceTexdef(SurfaceShader& shader, const TextureProjection& projection) :
	_shader(shader),
	_projection(projection)
{}

const TextureProjection& FaceTexdef::getProjection() const
{
    return _projection;
}

TextureProjection& FaceTexdef::getProjection()
{
    return _projection;
}

void FaceTexdef::setTexdef(const TextureProjection& projection)
{
	_projection.assign(projection);
}

void FaceTexdef::shift(float s, float t) 
{
	_projection.shift(s, t);
}

void FaceTexdef::scale(float s, float t)
{
	_projection.scale(s, t, _shader.getWidth(), _shader.getHeight());
}

void FaceTexdef::rotate(float angle)
{
	_projection.rotate(angle, _shader.getWidth(), _shader.getHeight());
}

void FaceTexdef::fit(const Vector3& normal, const Winding& winding, float s_repeat, float t_repeat) 
{
	_projection.fitTexture(_shader.getWidth(), _shader.getHeight(), normal, winding, s_repeat, t_repeat);
}

void FaceTexdef::flipTexture(unsigned int flipAxis)
{
	_projection.flipTexture(flipAxis);
}

void FaceTexdef::alignTexture(EAlignType align, const Winding& winding)
{
	_projection.alignTexture(align, winding);
}

void FaceTexdef::emitTextureCoordinates(Winding& winding, const Vector3& normal, const Matrix4& localToWorld) 
{
	_projection.emitTextureCoordinates(winding, normal, localToWorld);
}

void FaceTexdef::setBasis(const Vector3& normal)
{
	Matrix4 basis;
	Normal_GetTransform(normal, basis);
}

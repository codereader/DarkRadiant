#include "FaceTexDef.h"

// Constructor
FaceTexdef::FaceTexdef(FaceShader& shader, const TextureProjection& projection, bool projectionInitialised) :
	m_shader(shader),
	m_projection(projection),
	m_projectionInitialised(projectionInitialised),
	m_scaleApplied(false) 
{
	m_shader.attach(*this);
}

// Destructor
FaceTexdef::~FaceTexdef() {
	m_shader.detach(*this);
}

void FaceTexdef::addScale() {
	ASSERT_MESSAGE(!m_scaleApplied, "texture scale aready added");
	m_scaleApplied = true;
	m_projection.m_brushprimit_texdef.addScale(m_shader.width(), m_shader.height());
}

void FaceTexdef::removeScale() {
	ASSERT_MESSAGE(m_scaleApplied, "texture scale aready removed");
	m_scaleApplied = false;
	m_projection.m_brushprimit_texdef.removeScale(m_shader.width(), m_shader.height());
}

void FaceTexdef::realiseShader() {
	if (m_projectionInitialised && !m_scaleApplied) {
		addScale();
	}
}

void FaceTexdef::unrealiseShader() {
	if (m_projectionInitialised && m_scaleApplied) {
		removeScale();
	}
}

void FaceTexdef::setTexdef(const TextureProjection& projection) {
	removeScale();
	m_projection.assign(projection);
	addScale();
}

void FaceTexdef::shift(float s, float t) {
	ASSERT_MESSAGE(m_projection.m_texdef.isSane(), "FaceTexdef::shift: bad texdef");
	removeScale();
	m_projection.shift(s, t);
	addScale();
}

void FaceTexdef::scale(float s, float t) {
	removeScale();
	m_projection.scale(s, t);
	addScale();
}

void FaceTexdef::rotate(float angle) {
	removeScale();
	m_projection.rotate(angle);
	addScale();
}

void FaceTexdef::fit(const Vector3& normal, const Winding& winding, float s_repeat, float t_repeat) {
	m_projection.fitTexture(m_shader.width(), m_shader.height(), normal, winding, s_repeat, t_repeat);
}

void FaceTexdef::flipTexture(unsigned int flipAxis) {
	m_projection.flipTexture(flipAxis);
}

void FaceTexdef::emitTextureCoordinates(Winding& winding, const Vector3& normal, const Matrix4& localToWorld) {
	// old code // m_projection.emitTextureCoordinates(m_shader.width(), m_shader.height(), winding, normal, localToWorld);
	m_projection.emitTextureCoordinates(winding, normal, localToWorld);
}

void FaceTexdef::transform(const Plane3& plane, const Matrix4& matrix) {
	removeScale();
	m_projection.transformLocked(m_shader.width(), m_shader.height(), plane, matrix);
	addScale();
}

TextureProjection FaceTexdef::normalised() const {
	BrushPrimitTexDef tmp(m_projection.m_brushprimit_texdef);
	tmp.removeScale(m_shader.width(), m_shader.height());
	return TextureProjection(m_projection.m_texdef, tmp, m_projection.m_basis_s, m_projection.m_basis_t);
}

void FaceTexdef::setBasis(const Vector3& normal) {
	Matrix4 basis;
	Normal_GetTransform(normal, basis);
	m_projection.m_basis_s = Vector3(basis.xx(), basis.yx(), basis.zx());
	m_projection.m_basis_t = Vector3(-basis.xy(), -basis.yy(), -basis.zy());
}

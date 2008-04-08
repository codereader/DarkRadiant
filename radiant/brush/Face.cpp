#include "Face.h"

#include "ifilter.h"
#include "irenderable.h"

#include "shaderlib.h"
#include "Winding.h"
#include "cullable.h"

#include "Brush.h"
#include "BrushModule.h"
#include "ui/surfaceinspector/SurfaceInspector.h"

Face::Face(FaceObserver* observer) :
	m_refcount(0),
	m_shader(texdef_name_default()),
	m_texdef(m_shader, TextureProjection(), false),
	m_observer(observer),
	m_undoable_observer(0),
	m_map(0)
{
	m_shader.attach(*this);
	m_plane.copy(Vector3(0, 0, 0), Vector3(64, 0, 0), Vector3(0, 64, 0));
	m_texdef.setBasis(m_plane.plane3().normal());
	planeChanged();
}

Face::Face(
	const Vector3& p0,
	const Vector3& p1,
	const Vector3& p2,
	const std::string& shader,
	const TextureProjection& projection,
	FaceObserver* observer
) :
	m_refcount(0),
	m_shader(shader),
	m_texdef(m_shader, projection),
	m_observer(observer),
	m_undoable_observer(0),
	m_map(0)
{
	m_shader.attach(*this);
	m_plane.copy(p0, p1, p2);
	m_texdef.setBasis(m_plane.plane3().normal());
	planeChanged();
}

Face::Face(const Face& other, FaceObserver* observer) :
	m_refcount(0),
	m_shader(other.m_shader.getShader(), other.m_shader.m_flags),
	m_texdef(m_shader, other.getTexdef().normalised()),
	m_observer(observer),
	m_undoable_observer(0),
	m_map(0)
{
	m_shader.attach(*this);
	m_plane.copy(other.m_plane);
	planepts_assign(m_move_planepts, other.m_move_planepts);
	m_texdef.setBasis(m_plane.plane3().normal());
	planeChanged();
}

Face::~Face() {
	m_shader.detach(*this);
}

void Face::planeChanged() {
	revertTransform();
	m_observer->planeChanged();
}

void Face::realiseShader() {
	m_observer->shaderChanged();
}

void Face::unrealiseShader() {
}

void Face::instanceAttach(MapFile* map) {
	m_shader.instanceAttach();
	m_map = map;
	m_undoable_observer = GlobalUndoSystem().observer(this);
}

void Face::instanceDetach(MapFile* map) {
	m_undoable_observer = 0;
	GlobalUndoSystem().release(this);
	m_map = 0;
	m_shader.instanceDetach();
}

void Face::render(RenderStateFlags state) const {
	m_winding.draw(state);
}

void Face::undoSave() {
	if (m_map != 0) {
		m_map->changed();
	}
	
	if (m_undoable_observer != 0) {
		m_undoable_observer->save(this);
	}
}

// undoable
UndoMemento* Face::exportState() const {
	return new SavedState(*this);
}

void Face::importState(const UndoMemento* data) {
	undoSave();

	static_cast<const SavedState*>(data)->exportState(*this);

	planeChanged();
	m_observer->connectivityChanged();
	texdefChanged();
	m_observer->shaderChanged();
}

void Face::flipWinding() {
	m_plane.reverse();
	planeChanged();
}

bool Face::intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const {
	return volume.TestPlane(Plane3(plane3().normal(), -plane3().dist()), localToWorld);
}

void Face::render(Renderer& renderer, const Matrix4& localToWorld) const {
	// Submit this face to the Renderer only if its shader is not filtered
	if (m_shader.state()->getIShader()->isVisible()) {
		renderer.SetState(m_shader.state(), Renderer::eFullMaterials);
		renderer.addRenderable(*this, localToWorld);
	}
}

void Face::transform(const Matrix4& matrix, bool mirror) {
	if (GlobalBrush()->textureLockEnabled()) {
		m_texdefTransformed.transformLocked(m_shader.width(), m_shader.height(), m_plane.plane3(), matrix);
	}

	// Transform the FacePlane using the given matrix
	m_planeTransformed.transform(matrix, mirror);
	m_observer->planeChanged();
	updateWinding();
}

void Face::assign_planepts(const PlanePoints planepts) {
	m_planeTransformed.copy(planepts[0], planepts[1], planepts[2]);
	m_observer->planeChanged();
	updateWinding();
}

/// \brief Reverts the transformable state of the brush to identity. 
void Face::revertTransform() {
	m_planeTransformed = m_plane;
	planepts_assign(m_move_planeptsTransformed, m_move_planepts);
	m_texdefTransformed = m_texdef.m_projection;
	updateWinding();
}

void Face::freezeTransform() {
	undoSave();
	m_plane = m_planeTransformed;
	planepts_assign(m_move_planepts, m_move_planeptsTransformed);
	m_texdef.m_projection = m_texdefTransformed;
	updateWinding();
}

void Face::updateWinding() {
	m_winding.updateNormals(m_plane.plane3().normal());
}

void Face::update_move_planepts_vertex(std::size_t index, PlanePoints planePoints) {
	std::size_t numpoints = getWinding().numpoints;
	ASSERT_MESSAGE(index < numpoints, "update_move_planepts_vertex: invalid index");
	
	std::size_t opposite = getWinding().opposite(index);
	std::size_t adjacent = getWinding().wrap(opposite + numpoints - 1);
	planePoints[0] = getWinding()[opposite].vertex;
	planePoints[1] = getWinding()[index].vertex;
	planePoints[2] = getWinding()[adjacent].vertex;
	// winding points are very inaccurate, so they must be quantised before using them to generate the face-plane
	planepts_quantise(planePoints, GRID_MIN);
}

void Face::snapto(float snap) {
	if (contributes()) {
		PlanePoints planePoints;
		update_move_planepts_vertex(0, planePoints);
		vector3_snap(planePoints[0], snap);
		vector3_snap(planePoints[1], snap);
		vector3_snap(planePoints[2], snap);
		assign_planepts(planePoints);
		freezeTransform();
		SceneChangeNotify();
		if (!m_plane.plane3().isValid()) {
			globalErrorStream() << "WARNING: invalid plane after snap to grid\n";
		}
	}
}

void Face::testSelect(SelectionTest& test, SelectionIntersection& best) {
	m_winding.testSelect(test, best);
}

void Face::testSelect_centroid(SelectionTest& test, SelectionIntersection& best) {
	test.TestPoint(m_centroid, best);
}

void Face::shaderChanged() {
	EmitTextureCoordinates();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
	m_observer->shaderChanged();
	planeChanged();
	SceneChangeNotify();
}

const std::string& Face::GetShader() const {
	return m_shader.getShader();
}
void Face::SetShader(const std::string& name) {
	undoSave();
	m_shader.setShader(name);
	shaderChanged();
}

void Face::revertTexdef() {
	m_texdefTransformed = m_texdef.m_projection;
}

void Face::texdefChanged() {
	revertTexdef();
	EmitTextureCoordinates();
	// Update the Texture Tools
	ui::SurfaceInspector::Instance().update();
}

void Face::GetTexdef(TextureProjection& projection) const {
	projection = m_texdef.normalised();
}

void Face::SetTexdef(const TextureProjection& projection) {
	undoSave();
	m_texdef.setTexdef(projection);
	texdefChanged();
}

void Face::GetFlags(ContentsFlagsValue& flags) const {
	flags = m_shader.getFlags();
}

void Face::SetFlags(const ContentsFlagsValue& flags) {
	undoSave();
	m_shader.setFlags(flags);
	m_observer->shaderChanged();
}

void Face::ShiftTexdef(float s, float t) {
	undoSave();
	m_texdef.shift(s, t);
	texdefChanged();
}

void Face::ScaleTexdef(float s, float t) {
	undoSave();
	m_texdef.scale(s, t);
	texdefChanged();
}

void Face::RotateTexdef(float angle) {
	undoSave();
	m_texdef.rotate(angle);
	texdefChanged();
}

void Face::FitTexture(float s_repeat, float t_repeat) {
	undoSave();
	m_texdef.fit(m_plane.plane3().normal(), m_winding, s_repeat, t_repeat);
	texdefChanged();
}

void Face::flipTexture(unsigned int flipAxis) {
	undoSave();
	m_texdef.flipTexture(flipAxis);
	texdefChanged();
}

void Face::EmitTextureCoordinates() {
	//m_texdefTransformed.emitTextureCoordinates(m_shader.width(), m_shader.height(), m_winding, plane3().normal(), g_matrix4_identity);
	m_texdefTransformed.emitTextureCoordinates(m_winding, plane3().normal(), Matrix4::getIdentity());
}

const Vector3& Face::centroid() const {
	return m_centroid;
}

void Face::construct_centroid() {
	// Take the plane and let the winding calculate the centroid
	m_centroid = m_winding.centroid(plane3());
}

const Winding& Face::getWinding() const {
	return m_winding;
}
Winding& Face::getWinding() {
	return m_winding;
}

const Plane3& Face::plane3() const {
	m_observer->evaluateTransform();
	return m_planeTransformed.plane3();
}

FacePlane& Face::getPlane() {
	return m_plane;
}
const FacePlane& Face::getPlane() const {
	return m_plane;
}

FaceTexdef& Face::getTexdef() {
	return m_texdef;
}
const FaceTexdef& Face::getTexdef() const {
	return m_texdef;
}
FaceShader& Face::getShader() {
	return m_shader;
}
const FaceShader& Face::getShader() const {
	return m_shader;
}

bool Face::contributes() const {
	return m_winding.numpoints > 2;
}

bool Face::is_bounded() const {
	for (Winding::const_iterator i = m_winding.begin(); i != m_winding.end(); ++i) {
		if (i->adjacent == c_brush_maxFaces) {
			return false;
		}
	}
	return true;
}

void Face::normaliseTexture() {
	undoSave();
	
	Winding::const_iterator nearest = m_winding.begin();
		
	// Find the vertex with the minimal distance to the origin  
	for (Winding::const_iterator i = m_winding.begin(); i != m_winding.end(); ++i) {
		if (nearest->texcoord.getLength() > i->texcoord.getLength()) {
			nearest = i;
		}
	}
	
	Vector2 texcoord = nearest->texcoord;
	
	// The floored values
	Vector2 floored(floor(fabs(texcoord[0])), floor(fabs(texcoord[1])));
	
	// The signs of the original texcoords (needed to know which direction it should be shifted)
	Vector2 sign(texcoord[0]/fabs(texcoord[0]), texcoord[1]/fabs(texcoord[1]));
	
	Vector2 shift;
	shift[0] = (fabs(texcoord[0]) > 1.0E-4) ? -floored[0] * sign[0] * m_texdef.m_shader.width() : 0.0f;
	shift[0] = (fabs(texcoord[1]) > 1.0E-4) ? -floored[1] * sign[1] * m_texdef.m_shader.height() : 0.0f;
	
	// Shift the texture (note the minus sign, the FaceTexDef negates it yet again). 
	m_texdef.shift(-shift[0], shift[1]);
	
	texdefChanged();
}

// ---------------------------------------------------------------------------------------

QuantiseFunc Face::m_quantise;

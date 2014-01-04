#include "Face.h"

#include "ivolumetest.h"
#include "ifilter.h"
#include "itextstream.h"
#include "irenderable.h"

#include "shaderlib.h"
#include "Winding.h"

#include "Brush.h"
#include "BrushModule.h"
#include "ui/surfaceinspector/SurfaceInspector.h"

Face::Face(Brush& owner, FaceObserver* observer) :
    _owner(owner),
    _faceShader(*this, texdef_name_default()),
    m_texdef(_faceShader, TextureProjection(), false),
    m_observer(observer),
    _undoStateSaver(NULL),
    m_map(0),
    _faceIsVisible(true)
{
    _faceShader.attachObserver(*this);
    m_plane.initialiseFromPoints(
        Vector3(0, 0, 0), Vector3(64, 0, 0), Vector3(0, 64, 0)
    );
    m_texdef.setBasis(m_plane.getPlane().normal());
    planeChanged();
    shaderChanged();
}

Face::Face(
    Brush& owner,
    const Vector3& p0,
    const Vector3& p1,
    const Vector3& p2,
    const std::string& shader,
    const TextureProjection& projection,
    FaceObserver* observer
) :
    _owner(owner),
    _faceShader(*this, shader),
    m_texdef(_faceShader, projection),
    m_observer(observer),
    _undoStateSaver(NULL),
    m_map(0),
    _faceIsVisible(true)
{
    _faceShader.attachObserver(*this);
    m_plane.initialiseFromPoints(p0, p1, p2);
    m_texdef.setBasis(m_plane.getPlane().normal());
    planeChanged();
    shaderChanged();
}

Face::Face(Brush& owner, const Plane3& plane, FaceObserver* observer) :
    _owner(owner),
    _faceShader(*this, ""),
    m_texdef(_faceShader, TextureProjection()),
    m_observer(observer),
    _undoStateSaver(NULL),
    m_map(NULL),
    _faceIsVisible(true)
{
    _faceShader.attachObserver(*this);
    m_plane.setPlane(plane);
    m_texdef.setBasis(m_plane.getPlane().normal());
    planeChanged();
    shaderChanged();
}

Face::Face(Brush& owner, const Plane3& plane, const Matrix4& texdef,
           const std::string& shader, FaceObserver* observer) :
    _owner(owner),
    _faceShader(*this, shader),
    m_texdef(_faceShader, TextureProjection()),
    m_observer(observer),
    _undoStateSaver(NULL),
    m_map(NULL),
    _faceIsVisible(true)
{
    _faceShader.attachObserver(*this);
    m_plane.setPlane(plane);
    m_texdef.setBasis(m_plane.getPlane().normal());

    m_texdef.m_projection.m_brushprimit_texdef = BrushPrimitTexDef(texdef);
    m_texdef.m_projectionInitialised = true;
    m_texdef.m_scaleApplied = true;

    planeChanged();
    shaderChanged();
}

Face::Face(Brush& owner, const Face& other, FaceObserver* observer) :
    IFace(other),
    IUndoable(other),
    FaceShader::Observer(other),
    _owner(owner),
    m_plane(other.m_plane),
    _faceShader(*this, other._faceShader.getMaterialName(), other._faceShader.m_flags),
    m_texdef(_faceShader, other.getTexdef().normalised()),
    m_observer(observer),
    _undoStateSaver(NULL),
    m_map(0),
    _faceIsVisible(other._faceIsVisible)
{
    _faceShader.attachObserver(*this);
    planepts_assign(m_move_planepts, other.m_move_planepts);
    m_texdef.setBasis(m_plane.getPlane().normal());
    planeChanged();
}

Face::~Face() {
    _faceShader.detachObserver(*this);
}

Brush& Face::getBrush()
{
    return _owner;
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

void Face::instanceAttach(MapFile* map)
{
    _faceShader.setInUse(true);
    m_map = map;
	_undoStateSaver = GlobalUndoSystem().getStateSaver(*this);
}

void Face::instanceDetach(MapFile* map)
{
    _undoStateSaver = NULL;
    GlobalUndoSystem().releaseStateSaver(*this);
    m_map = 0;
    _faceShader.setInUse(false);
}

void Face::undoSave() {
    if (m_map != 0) {
        m_map->changed();
    }

    if (_undoStateSaver != NULL)
	{
        _undoStateSaver->save(*this);
    }
}

// undoable
IUndoMementoPtr Face::exportState() const
{
    return IUndoMementoPtr(new SavedState(*this));
}

void Face::importState(const IUndoMementoPtr& data)
{
    undoSave();

	std::static_pointer_cast<SavedState>(data)->exportState(*this);

    planeChanged();
    m_observer->connectivityChanged();
    texdefChanged();
    m_observer->shaderChanged();
}

void Face::flipWinding() {
    m_plane.reverse();
    planeChanged();
}

bool Face::intersectVolume(const VolumeTest& volume) const
{
    if (!m_winding.empty())
    {
        const Plane3& plane = m_planeTransformed.getPlane();
        return volume.TestPlane(Plane3(plane.normal(), -plane.dist()));
    }
    else
    {
        // Empty winding, return false
        return false;
    }
}

bool Face::intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const
{
    if (m_winding.size() > 0)
    {
        return volume.TestPlane(Plane3(plane3().normal(), -plane3().dist()), localToWorld);
    }
    else
    {
        // Empty winding, return false
        return false;
    }
}

void Face::submitRenderables(RenderableCollector& collector,
                             const Matrix4& localToWorld,
                             const IRenderEntity& entity) const
{
    collector.SetState(_faceShader.getGLShader(), RenderableCollector::eFullMaterials);
    collector.addRenderable(m_winding, localToWorld, entity);
}

void Face::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    _faceShader.setRenderSystem(renderSystem);

    // Update the visibility flag, we might have switched shaders
    const ShaderPtr& shader = _faceShader.getGLShader();

    if (shader)
    {
        _faceIsVisible = shader->getMaterial()->isVisible();
    }
    else
    {
        _faceIsVisible = false; // no shader => not visible
    }
}

void Face::translate(const Vector3& translation)
{
    m_planeTransformed.translate(translation);
    m_observer->planeChanged();
    updateWinding();
}

void Face::transform(const Matrix4& matrix, bool mirror)
{
    if (GlobalBrush().textureLockEnabled()) {
        m_texdefTransformed.transformLocked(_faceShader.width(), _faceShader.height(), m_plane.getPlane(), matrix);
    }

    // Transform the FacePlane using the given matrix
    m_planeTransformed.transform(matrix);
    m_observer->planeChanged();
    updateWinding();
}

void Face::assign_planepts(const PlanePoints planepts)
{
    m_planeTransformed.initialiseFromPoints(
        planepts[0], planepts[1], planepts[2]
    );
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
    m_winding.updateNormals(m_plane.getPlane().normal());
}

void Face::update_move_planepts_vertex(std::size_t index, PlanePoints planePoints) {
    std::size_t numpoints = getWinding().size();
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
        planePoints[0].snap(snap);
        planePoints[1].snap(snap);
        planePoints[2].snap(snap);
        assign_planepts(planePoints);
        freezeTransform();
        SceneChangeNotify();
        if (!m_plane.getPlane().isValid()) {
            rError() << "WARNING: invalid plane after snap to grid\n";
        }
    }
}

void Face::testSelect(SelectionTest& test, SelectionIntersection& best) {
    m_winding.testSelect(test, best);
}

void Face::testSelect_centroid(SelectionTest& test, SelectionIntersection& best) {
    test.TestPoint(m_centroid, best);
}

void Face::shaderChanged()
{
    EmitTextureCoordinates();
    m_observer->shaderChanged();

    // Update the visibility flag, but leave out the contributes() check
    const ShaderPtr& shader = getFaceShader().getGLShader();

    if (shader)
    {
        _faceIsVisible = shader->getMaterial()->isVisible();
    }
    else
    {
        _faceIsVisible = false; // no shader => not visible
    }

    planeChanged();
    SceneChangeNotify();
}

const std::string& Face::getShader() const
{
    return _faceShader.getMaterialName();
}

void Face::setShader(const std::string& name)
{
    undoSave();
    _faceShader.setMaterialName(name);
    shaderChanged();
}

void Face::revertTexdef() {
    m_texdefTransformed = m_texdef.m_projection;
}

void Face::texdefChanged()
{
    revertTexdef();
    EmitTextureCoordinates();

    // Update the Texture Tools
    ui::SurfaceInspector::update();
}

void Face::GetTexdef(TextureProjection& projection) const {
    projection = m_texdef.normalised();
}

void Face::SetTexdef(const TextureProjection& projection) {
    undoSave();
    m_texdef.setTexdef(projection);
    texdefChanged();
}

void Face::applyShaderFromFace(const Face& other) {
    // Retrieve the textureprojection from the source face
    TextureProjection projection;
    other.GetTexdef(projection);

    setShader(other.getShader());
    SetTexdef(projection);
    SetFlags(other.getFaceShader().m_flags);

    // The list of shared vertices
    std::vector<Winding::const_iterator> thisVerts, otherVerts;

    // Let's see whether this face is sharing any 3D coordinates with the other one
    for (Winding::const_iterator i = other.m_winding.begin(); i != other.m_winding.end(); ++i) {
        for (Winding::const_iterator j = m_winding.begin(); j != m_winding.end(); ++j) {
            // Check if the vertices are matching
            if (j->vertex.isEqual(i->vertex, 0.001))
            {
                // Match found, add to list
                thisVerts.push_back(j);
                otherVerts.push_back(i);
            }
        }
    }

    if (thisVerts.empty() || thisVerts.size() != otherVerts.size()) {
        return; // nothing to do
    }

    // Calculate the distance in texture space of the first shared vertices
    Vector2 dist = thisVerts[0]->texcoord - otherVerts[0]->texcoord;

    // Scale the translation (ShiftTexDef() is scaling this down again, yes this is weird).
    dist[0] *= getFaceShader().width();
    dist[1] *= getFaceShader().height();

    // Shift the texture to match
    shiftTexdef(dist.x(), dist.y());
}

void Face::GetFlags(ContentsFlagsValue& flags) const {
    flags = _faceShader.getFlags();
}

void Face::SetFlags(const ContentsFlagsValue& flags) {
    undoSave();
    _faceShader.setFlags(flags);
    m_observer->shaderChanged();
}

void Face::shiftTexdef(float s, float t) {
    undoSave();
    m_texdef.shift(s, t);
    texdefChanged();
}

void Face::scaleTexdef(float s, float t) {
    undoSave();
    m_texdef.scale(s, t);
    texdefChanged();
}

void Face::rotateTexdef(float angle) {
    undoSave();
    m_texdef.rotate(angle);
    texdefChanged();
}

void Face::fitTexture(float s_repeat, float t_repeat) {
    undoSave();
    m_texdef.fit(m_plane.getPlane().normal(), m_winding, s_repeat, t_repeat);
    texdefChanged();
}

void Face::flipTexture(unsigned int flipAxis) {
    undoSave();
    m_texdef.flipTexture(flipAxis);
    texdefChanged();
}

void Face::alignTexture(EAlignType align)
{
    undoSave();
    m_texdef.alignTexture(align, m_winding);
    texdefChanged();
}

void Face::EmitTextureCoordinates() {
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
    return m_planeTransformed.getPlane();
}

const Plane3& Face::getPlane3() const
{
    return m_plane.getPlane();
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

Matrix4 Face::getTexDefMatrix() const
{
    return m_texdef.m_projection.m_brushprimit_texdef.getTransform();
}

FaceShader& Face::getFaceShader() {
    return _faceShader;
}
const FaceShader& Face::getFaceShader() const {
    return _faceShader;
}

bool Face::contributes() const {
    return m_winding.size() > 2;
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

void Face::updateFaceVisibility()
{
    _faceIsVisible = contributes() && getFaceShader().getGLShader()->getMaterial()->isVisible();
}

// ---------------------------------------------------------------------------------------

QuantiseFunc Face::m_quantise = quantiseFloating;

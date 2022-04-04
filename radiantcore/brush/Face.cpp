#include "Face.h"

#include "ivolumetest.h"
#include "ifilter.h"
#include "itextstream.h"
#include "irenderable.h"

#include "math/Matrix3.h"
#include "shaderlib.h"
#include "texturelib.h"
#include "Winding.h"
#include "selection/algorithm/Texturing.h"

#include "Brush.h"
#include "BrushNode.h"
#include "BrushModule.h"

// The structure that is saved in the undostack
class Face::SavedState final :
    public IUndoMemento
{
public:
    FacePlane::SavedState _planeState;
    TextureProjection _texdefState;
    std::string _materialName;

    SavedState(const Face& face) :
        _planeState(face.getPlane()),
        _texdefState(face.getProjection()),
        _materialName(face.getShader())
    {}
};

Face::Face(Brush& owner) :
    _owner(owner),
    _shader(texdef_name_default(), _owner.getBrushNode().getRenderSystem()),
    _undoStateSaver(nullptr),
    _faceIsVisible(true),
    _windingSurfaceSolid(m_winding, false),
    _windingSurfaceWireframe(m_winding, true)
{
    setupSurfaceShader();

    m_plane.initialiseFromPoints(
        Vector3(0, 0, 0), Vector3(64, 0, 0), Vector3(0, 64, 0)
    );
    planeChanged();
    shaderChanged();
}

Face::Face(
    Brush& owner,
    const Vector3& p0,
    const Vector3& p1,
    const Vector3& p2,
    const std::string& shader,
    const TextureProjection& projection
) :
    _owner(owner),
    _shader(shader, _owner.getBrushNode().getRenderSystem()),
    _texdef(projection),
    _undoStateSaver(nullptr),
    _faceIsVisible(true),
    _windingSurfaceSolid(m_winding, false),
    _windingSurfaceWireframe(m_winding, true)
{
    setupSurfaceShader();
    m_plane.initialiseFromPoints(p0, p1, p2);
    planeChanged();
    shaderChanged();
}

Face::Face(Brush& owner, const Plane3& plane) :
    _owner(owner),
    _shader("", _owner.getBrushNode().getRenderSystem()),
    _undoStateSaver(nullptr),
    _faceIsVisible(true),
    _windingSurfaceSolid(m_winding, false),
    _windingSurfaceWireframe(m_winding, true)
{
    setupSurfaceShader();
    m_plane.setPlane(plane);
    planeChanged();
    shaderChanged();
}

Face::Face(Brush& owner, const Plane3& plane, const Matrix3& textureProjection, const std::string& material) :
    _owner(owner),
    _shader(material, _owner.getBrushNode().getRenderSystem()),
    _undoStateSaver(nullptr),
    _faceIsVisible(true),
    _windingSurfaceSolid(m_winding, false),
    _windingSurfaceWireframe(m_winding, true)
{
    setupSurfaceShader();
    m_plane.setPlane(plane);

    _texdef.setTransform(textureProjection);

    planeChanged();
    shaderChanged();
}

Face::Face(Brush& owner, const Face& other) :
    IFace(other),
    IUndoable(other),
    _owner(owner),
    m_plane(other.m_plane),
    _shader(other._shader.getMaterialName(), _owner.getBrushNode().getRenderSystem()),
    _texdef(other.getProjection()),
    _undoStateSaver(nullptr),
    _faceIsVisible(other._faceIsVisible),
    _windingSurfaceSolid(m_winding, false),
    _windingSurfaceWireframe(m_winding, true)
{
    setupSurfaceShader();
    planepts_assign(m_move_planepts, other.m_move_planepts);
    planeChanged();
}

Face::~Face()
{
    _surfaceShaderRealised.disconnect();
    _sigDestroyed.emit();
    _sigDestroyed.clear();

    // Deallocate the winding surface
    clearRenderables();
}

sigc::signal<void>& Face::signal_faceDestroyed()
{
    return _sigDestroyed;
}

void Face::setupSurfaceShader()
{
    _surfaceShaderRealised = _shader.signal_Realised().connect(
        sigc::mem_fun(*this, &Face::realiseShader));

    // If we're already in realised state, call realiseShader right away
    if (_shader.isRealised())
    {
        realiseShader();
    }
}

IBrush& Face::getBrush()
{
    return _owner;
}

Brush& Face::getBrushInternal()
{
    return _owner;
}

void Face::planeChanged()
{
    updateRenderables();

    revertTransform();
    _owner.onFacePlaneChanged();
}

void Face::realiseShader()
{
    _owner.onFaceShaderChanged();
}

void Face::connectUndoSystem(IUndoSystem& undoSystem)
{
    assert(!_undoStateSaver);

    _shader.setInUse(true);

    updateRenderables();

    _undoStateSaver = undoSystem.getStateSaver(*this);
}

void Face::disconnectUndoSystem(IUndoSystem& undoSystem)
{
    assert(_undoStateSaver);
    _undoStateSaver = nullptr;
    undoSystem.releaseStateSaver(*this);

    // Disconnect the renderable winding vertices from the scene
    clearRenderables();

    _shader.setInUse(false);
}

void Face::undoSave()
{
    if (_undoStateSaver)
    {
        _undoStateSaver->saveState();
    }
}

// undoable
IUndoMementoPtr Face::exportState() const
{
    return std::make_shared<SavedState>(*this);
}

void Face::importState(const IUndoMementoPtr& data)
{
    undoSave();

    auto state = std::static_pointer_cast<SavedState>(data);

    state->_planeState.exportState(getPlane());
    setShader(state->_materialName);
    _texdef = state->_texdefState;

    planeChanged();
    _owner.onFaceConnectivityChanged();
    texdefChanged();
    _owner.onFaceShaderChanged();
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

void Face::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    _shader.setRenderSystem(renderSystem);

    // Update the visibility flag, we might have switched shaders
    const ShaderPtr& shader = _shader.getGLShader();

    _faceIsVisible = shader && shader->getMaterial()->isVisible();

    clearRenderables();
}

void Face::transformTexDefLocked(const Matrix4& transform)
{
    Vector3 vertices[3] =
    {
        m_winding[0].vertex,
        m_winding[1].vertex,
        m_winding[2].vertex
    };

    Vector2 texcoords[3] =
    {
        m_winding[0].texcoord,
        m_winding[1].texcoord,
        m_winding[2].texcoord
    };

    // Transform the vertices
    vertices[0] = transform.transformPoint(vertices[0]);
    vertices[1] = transform.transformPoint(vertices[1]);
    vertices[2] = transform.transformPoint(vertices[2]);

    // Keep the texture coords, recalculate the texture projection
    m_texdefTransformed.calculateFromPoints(vertices, texcoords, m_planeTransformed.getPlane().normal());
}

void Face::translate(const Vector3& translation)
{
    m_planeTransformed.translate(translation);
    
    if (GlobalBrush().textureLockEnabled() && m_winding.size() >= 3)
    {
        transformTexDefLocked(Matrix4::getTranslation(translation));
    }

    _owner.onFacePlaneChanged();
    updateWinding();
}

void Face::transform(const Matrix4& transform)
{
    // Transform the FacePlane using the given matrix (before the tex def is recalculated)
    m_planeTransformed.transform(transform);

    if (GlobalBrush().textureLockEnabled() && m_winding.size() >= 3)
    {
        transformTexDefLocked(transform);
    }

    _owner.onFacePlaneChanged();
    updateWinding();
}

void Face::assign_planepts(const PlanePoints planepts)
{
    m_planeTransformed.initialiseFromPoints(
        planepts[0], planepts[1], planepts[2]
    );
    _owner.onFacePlaneChanged();
    updateWinding();
}

/// \brief Reverts the transformable state of the brush to identity.
void Face::revertTransform()
{
    m_planeTransformed = m_plane;
    planepts_assign(m_move_planeptsTransformed, m_move_planepts);
    m_texdefTransformed = _texdef;
    updateWinding();
    emitTextureCoordinates();
}

void Face::freezeTransform()
{
    undoSave();
    m_plane = m_planeTransformed;
    planepts_assign(m_move_planepts, m_move_planeptsTransformed);
    _texdef = m_texdefTransformed;
    updateWinding();
}

void Face::clearRenderables()
{
    _windingSurfaceSolid.clear();
    _windingSurfaceWireframe.clear();
}

void Face::updateRenderables()
{
    _windingSurfaceSolid.queueUpdate();
    _windingSurfaceWireframe.queueUpdate();

    _owner.onFaceNeedsRenderableUpdate();
}

void Face::updateWinding()
{
    updateRenderables();
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
    emitTextureCoordinates();
    _owner.onFaceShaderChanged();

    // Update the visibility flag, but leave out the contributes() check
    const ShaderPtr& shader = getFaceShader().getGLShader();
    _faceIsVisible = shader && shader->getMaterial()->isVisible();

    planeChanged(); // updates renderables too
    SceneChangeNotify();
}

const std::string& Face::getShader() const
{
    return _shader.getMaterialName();
}

void Face::setShader(const std::string& name)
{
    undoSave();

    auto ssr = getShiftScaleRotation();

    _shader.setMaterialName(name);

    // Adjust the scale to match the previous material
    auto newSsr = getShiftScaleRotation();

    newSsr.scale[0] = ssr.scale[0];
    newSsr.scale[1] = ssr.scale[1];

    setShiftScaleRotation(newSsr);

    shaderChanged();
}

void Face::revertTexdef()
{
    m_texdefTransformed = _texdef;
}

void Face::texdefChanged()
{
    revertTexdef();
    emitTextureCoordinates();
    updateRenderables();

    // Fire the signal to update the Texture Tools
    signal_texdefChanged().emit();
}

const TextureProjection& Face::getProjection() const
{
    return _texdef;
}

TextureProjection& Face::getProjection()
{
    return _texdef;
}

Matrix3 Face::getProjectionMatrix() const
{
    return getProjection().getMatrix();
}

void Face::setProjectionMatrix(const Matrix3& projection)
{
    getProjection().setTransform(projection);
    texdefChanged();
}

void Face::GetTexdef(TextureProjection& projection) const
{
    projection = _texdef;
}

void Face::SetTexdef(const TextureProjection& projection)
{
    undoSave();
    _texdef = projection;
    texdefChanged();
}

ShiftScaleRotation Face::getShiftScaleRotation() const
{
    return _texdef.getShiftScaleRotation(_shader.getWidth(), _shader.getHeight());
}

void Face::setShiftScaleRotation(const ShiftScaleRotation& ssr)
{
    undoSave();

    // Construct the matrix from the adjusted shift/scale/rotate values
    _texdef.setFromShiftScaleRotate(ssr, _shader.getWidth(), _shader.getHeight());

    texdefChanged();
}

Vector2 Face::getTexelScale() const
{
    auto imageWidth = _shader.getWidth();
    auto imageHeight = _shader.getHeight();

    auto textureMatrix = _texdef.getMatrix();

    // Multiplying the image dimensions onto the texture matrix yields
    // the base vectors in texel space. Take the length to get the covered texels per world unit
    return Vector2(
        Vector2(textureMatrix.xx() * imageWidth, textureMatrix.xy() * imageHeight).getLength(),
        Vector2(textureMatrix.yx() * imageWidth, textureMatrix.yy() * imageHeight).getLength()
    );
}

float Face::getTextureAspectRatio() const
{
    return _shader.getTextureAspectRatio();
}

// Returns the index pair forming an edge, keeping the winding direction intact
inline std::pair<std::size_t, std::size_t> getEdgeIndexPair(std::size_t first, std::size_t second, std::size_t windingSize)
{
    if (first > second || second == windingSize - 1 && first == 0)
    {
        std::swap(first, second);
    }

    return std::make_pair(first, second);
}

void Face::applyShaderFromFace(const Face& other)
{
    undoSave();

    // Apply the material of the other face
    setShader(other.getShader());

    // Retrieve the textureprojection from the source face
    TextureProjection projection;
    other.GetTexdef(projection);

    // The list of shared vertices (other face index => this face index)
    std::vector<std::pair<std::size_t, std::size_t>> sharedVertices;

    // Let's see whether this face is sharing any 3D coordinates with the other one
    // It's important to iterate over ascending indices of the other face, since we need to keep the winding order
    for (std::size_t i = 0; i < other.m_winding.size(); ++i)
    {
        for (std::size_t j = 0; j < m_winding.size(); ++j)
        {
            // Check if the vertices are matching
            if (math::isNear(m_winding[j].vertex, other.m_winding[i].vertex, 0.001))
            {
                // Match found, add to list
                sharedVertices.emplace_back(std::make_pair(i, j));
                break;
            }
        }
    }

    // Do we have a shared edge?
    if (sharedVertices.size() == 2)
    {
        auto edgeIndices = getEdgeIndexPair(sharedVertices[0].first, sharedVertices[1].first, other.m_winding.size());

        // We wrap the texture around the shared edge, check the UV scale perpendicular to that edge
        auto edgeCenter = (other.m_winding[edgeIndices.first].vertex + other.m_winding[edgeIndices.second].vertex) * 0.5;

        // Construct an edge vector, following the winding direction
        auto edge = other.m_winding[edgeIndices.second].vertex - other.m_winding[edgeIndices.first].vertex;

        // Construct a vector that is orthogonal to the edge, pointing outwards
        auto outwardsDirection = edge.cross(other.m_planeTransformed.getPlane().normal());

        // Pick a point outside face, placing that orthogonal vector on the edge center
        auto extrapolatedPoint = edgeCenter + outwardsDirection;
        auto extrapolationLength = outwardsDirection.getLength();
        auto extrapolatedTexcoords = other.m_texdefTransformed.getTextureCoordsForVertex(
            extrapolatedPoint, other.m_planeTransformed.getPlane().normal(), Matrix4::getIdentity()
        );

        // Construct an edge vector on this target face, keeping the winding order
        edgeIndices = getEdgeIndexPair(sharedVertices[0].second, sharedVertices[1].second, m_winding.size());

        auto targetFaceEdge = m_winding[edgeIndices.second].vertex - m_winding[edgeIndices.first].vertex;
        auto inwardsDirection = -targetFaceEdge.cross(m_planeTransformed.getPlane().normal()).getNormalised();

        // Calculate a point on this face plane, with the same distance from the edge center as on the source face
        auto pointOnThisFacePlane = edgeCenter + inwardsDirection * extrapolationLength;

        // Now we have 3 vertices and 3 texcoords to calculate the matching texdef
        Vector3 vertices[3] =
        {
            m_winding[sharedVertices[0].second].vertex,
            m_winding[sharedVertices[1].second].vertex,
            pointOnThisFacePlane
        };

        // Use the shared texcoords we found on the other face, and the third one we calculated
        Vector2 texcoords[3] =
        {
            other.m_winding[sharedVertices[0].first].texcoord,
            other.m_winding[sharedVertices[1].first].texcoord,
            extrapolatedTexcoords
        };

        setTexDefFromPoints(vertices, texcoords);
        _texdef = m_texdefTransformed; // freeze that matrix
        return;
    }
    else
    {
        // Just use the other projection, as-is
        SetTexdef(projection);
    }
}

void Face::setTexDefFromPoints(const Vector3 points[3], const Vector2 uvs[3])
{
    m_texdefTransformed.calculateFromPoints(points, uvs, getPlane3().normal());

    emitTextureCoordinates();

    // Fire the signal to update the Texture Tools
    signal_texdefChanged().emit();
}

void Face::shiftTexdef(float s, float t)
{
    undoSave();
    _texdef.shift(s, t);
    texdefChanged();
}

void Face::shiftTexdefByPixels(float sPixels, float tPixels)
{
    // Scale down the s,t translation using the active texture dimensions
    shiftTexdef(sPixels / _shader.getWidth(), tPixels / _shader.getHeight());
}

void Face::scaleTexdef(float sFactor, float tFactor)
{
    selection::algorithm::TextureScaler::ScaleFace(*this, { sFactor, tFactor });
}

void Face::rotateTexdef(float angle)
{
    selection::algorithm::TextureRotator::RotateFace(*this, degrees_to_radians(angle));
}

void Face::fitTexture(float s_repeat, float t_repeat) {
    undoSave();
    _texdef.fitTexture(_shader.getWidth(), _shader.getHeight(), m_plane.getPlane().normal(), m_winding, s_repeat, t_repeat);
    texdefChanged();
}

void Face::flipTexture(unsigned int flipAxis)
{
    selection::algorithm::TextureFlipper::FlipFace(*this, flipAxis);
}

void Face::alignTexture(AlignEdge align)
{
    undoSave();
    _texdef.alignTexture(align, m_winding);
    texdefChanged();
}

void Face::emitTextureCoordinates() 
{
    m_texdefTransformed.emitTextureCoordinates(m_winding, m_planeTransformed.getPlane().normal(), Matrix4::getIdentity());
}

void Face::applyDefaultTextureScale()
{
    _texdef = TextureProjection::ConstructDefault(_shader.getWidth(), _shader.getHeight());
    texdefChanged();
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

render::RenderableWinding& Face::getWindingSurfaceSolid()
{
    return _windingSurfaceSolid;
}

render::RenderableWinding& Face::getWindingSurfaceWireframe()
{
    return _windingSurfaceWireframe;
}

const Plane3& Face::plane3() const
{
    _owner.onFaceEvaluateTransform();
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

SurfaceShader& Face::getFaceShader() {
    return _shader;
}
const SurfaceShader& Face::getFaceShader() const {
    return _shader;
}

bool Face::contributes() const {
    return m_winding.size() > 2;
}

bool Face::is_bounded() const {
    for (Winding::const_iterator i = m_winding.begin(); i != m_winding.end(); ++i) {
        if (i->adjacent == brush::c_brush_maxFaces) {
            return false;
        }
    }
    return true;
}

void Face::normaliseTexture()
{
    selection::algorithm::TextureNormaliser::NormaliseFace(*this);
}

bool Face::isVisible() const
{
    return _faceIsVisible;
}

void Face::onBrushVisibilityChanged(bool visible)
{
    if (!visible)
    {
        // Disconnect our renderable when the owning brush goes invisible
        clearRenderables();
    }
    else
    {
        // Update the vertex buffers next time we need to render
        updateRenderables();
    }
}

void Face::updateFaceVisibility()
{
    auto newValue = contributes() && getFaceShader().getGLShader()->getMaterial()->isVisible();
    
    // Notify the owning brush if the value changes
    if (newValue != _faceIsVisible)
    {
        _faceIsVisible = newValue;
    }
}

sigc::signal<void>& Face::signal_texdefChanged()
{
    static sigc::signal<void> _sigTexdefChanged;
    return _sigTexdefChanged;
}

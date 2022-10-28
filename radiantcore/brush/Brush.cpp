#include "Brush.h"

#include "math/Frustum.h"
#include "irenderable.h"
#include "itextstream.h"
#include "shaderlib.h"

#include "BrushModule.h"
#include "BrushNode.h"
#include "Face.h"
#include "FixedWinding.h"
#include "math/Ray.h"

#include <functional>

namespace {
    /// \brief Returns true if edge (\p x, \p y) is smaller than the epsilon used to classify winding points against a plane.
    inline bool Edge_isDegenerate(const Vector3& x, const Vector3& y) {
        return (y - x).getLengthSquared() < (ON_EPSILON * ON_EPSILON);
    }

    inline Vector3::ElementType max_extent_2d(const Vector3& extents, int axis)
    {
        switch(axis)
        {
            case 0: return std::max(extents[1], extents[2]);
            case 1: return std::max(extents[0], extents[2]);
            default: return std::max(extents[0], extents[1]);
        }
    }

    inline Vector3::ElementType max_extent(const Vector3& extents)
    {
        return std::max(std::max(extents[0], extents[1]), extents[2]);
    }
}

Brush::Brush(BrushNode& owner) :
    _owner(owner),
    _undoStateSaver(nullptr),
    m_planeChanged(false),
    m_transformChanged(false),
	_detailFlag(Structural)
{
    // Make some space for a few faces
    reserve(6);
    onFacePlaneChanged();
}

Brush::Brush(BrushNode& owner, const Brush& other) :
    _owner(owner),
    _undoStateSaver(nullptr),
    m_planeChanged(false),
    m_transformChanged(false),
	_detailFlag(Structural)
{
    copy(other);
}

Brush::~Brush()
{
    ASSERT_MESSAGE(m_observers.empty(), "Brush::~Brush: observers still attached");
}

BrushNode& Brush::getBrushNode()
{
    return _owner;
}

IFace& Brush::getFace(std::size_t index)
{
    assert(index < m_faces.size());
    return *m_faces[index];
}

const IFace& Brush::getFace(std::size_t index) const
{
    assert(index < m_faces.size());
    return *m_faces[index];
}

IFace& Brush::addFace(const Plane3& plane)
{
    // Allocate a new Face
    undoSave();
    push_back(FacePtr(new Face(*this, plane)));

    return *m_faces.back();
}

IFace& Brush::addFace(const Plane3& plane, const Matrix3& textureProjection, const std::string& material)
{
    // Allocate a new Face
    undoSave();
    push_back(FacePtr(new Face(*this, plane, textureProjection, material)));

    return *m_faces.back();
}

void Brush::attach(BrushObserver& observer) {
    for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
        observer.push_back(*(*i));
    }

    for(SelectableEdges::iterator i = m_select_edges.begin(); i !=m_select_edges.end(); ++i) {
        observer.edge_push_back(*i);
    }

    for(SelectableVertices::iterator i = m_select_vertices.begin(); i != m_select_vertices.end(); ++i) {
        observer.vertex_push_back(*i);
    }

    m_observers.insert(&observer);
}

void Brush::detach(BrushObserver& observer)
{
    m_observers.erase(&observer);
}

void Brush::forEachFace(const std::function<void(Face&)>& functor) const
{
    for (const FacePtr& face : m_faces) functor(*face);
}

void Brush::forEachVisibleFace(const std::function<void(Face&)>& functor) const
{
    bool forceVisible = _owner.facesAreForcedVisible();

    for (const auto& face : m_faces)
    {
        if (forceVisible || face->isVisible())
        {
            functor(*face);
        }
    }
}

void Brush::connectUndoSystem(IUndoSystem& undoSystem)
{
    assert(_undoStateSaver == nullptr);

	_undoStateSaver = undoSystem.getStateSaver(*this);

    forEachFace([&](Face& face) { face.connectUndoSystem(undoSystem); });
}

void Brush::disconnectUndoSystem(IUndoSystem& undoSystem)
{
    assert(_undoStateSaver != nullptr);

    // Notify each face
    forEachFace([&](Face& face) { face.disconnectUndoSystem(undoSystem); });

    _undoStateSaver = nullptr;
    undoSystem.releaseStateSaver(*this);
}

void Brush::setShader(const std::string& newShader) {
    undoSave();

    for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
        (*i)->setShader(newShader);
    }
}

bool Brush::hasShader(const std::string& name) {
    // Traverse the faces
    for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
        if (shader_equal((*i)->getShader(), name)) {
            return true;
        }
    }

    // not found
    return false;
}

bool Brush::hasVisibleMaterial() const
{
    // Traverse the faces
    for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i)
    {
        if ((*i)->getFaceShader().getGLShader()->getMaterial()->isVisible())
        {
            return true; // return true on first visible material
        }
    }

    // no visible material
    return false;
}

void Brush::updateFaceVisibility()
{
    _owner.updateFaceVisibility();
}

void Brush::onFaceNeedsRenderableUpdate()
{
    _owner.onFaceNeedsRenderableUpdate();
}

Brush::DetailFlag Brush::getDetailFlag() const
{
	return _detailFlag;
}

void Brush::setDetailFlag(DetailFlag newValue)
{
	undoSave();

	_detailFlag = newValue;
}

BrushSplitType Brush::classifyPlane(const Plane3& plane) const
{
    evaluateBRep();

    BrushSplitType split;

    for (auto face : *this)
    {
        if (face->contributes())
        {
            split += face->getWinding().classifyPlane(plane);
        }
    }

    return split;
}

void Brush::evaluateBRep() const {
    if(m_planeChanged) {
        m_planeChanged = false;
        const_cast<Brush*>(this)->buildBRep();
    }
}

void Brush::transformChanged() {
    m_transformChanged = true;
    onFacePlaneChanged();
}

void Brush::evaluateTransform()
{
    if (m_transformChanged)
	{
        m_transformChanged = false;
        revertTransform();
        _owner.evaluateTransform();
    }
}

void Brush::aabbChanged()
{
    _owner.boundsChanged();
}

const AABB& Brush::localAABB() const {
    evaluateBRep();
    return m_aabb_local;
}

void Brush::translate(const Vector3& translation)
{
    std::for_each(m_faces.begin(), m_faces.end(),
                  std::bind(&Face::translate, std::placeholders::_1, translation));
    freezeTransform();
}

void Brush::transform(const Matrix4& matrix)
{
    for (const FacePtr& face : m_faces)
    {
        face->transform(matrix);
    }
}

void Brush::snapto(float snap) {
    for(Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
        (*i)->snapto(snap);
    }
}

void Brush::revertTransform() {
    for(Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
        (*i)->revertTransform();
    }
}

void Brush::freezeTransform() {
    for(Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
        (*i)->freezeTransform();
    }
}

/// \brief Returns the absolute index of the \p faceVertex.
std::size_t Brush::absoluteIndex(FaceVertexId faceVertex) {
    std::size_t index = 0;
    for(std::size_t i = 0; i < faceVertex.getFace(); ++i) {
        index += m_faces[i]->getWinding().size();
    }
    return index + faceVertex.getVertex();
}

void Brush::appendFaces(const Faces& other) {
    clear();
    for(Faces::const_iterator i = other.begin(); i != other.end(); ++i) {
        push_back(*i);
    }
}

void Brush::undoSave()
{
    if (_undoStateSaver != nullptr)
	{
        _undoStateSaver->saveState();
    }
}

IUndoMementoPtr Brush::exportState() const
{
    return IUndoMementoPtr(new BrushUndoMemento(m_faces, _detailFlag));
}

void Brush::importState(const IUndoMementoPtr& state)
{
    undoSave();

	BrushUndoMemento& memento = *std::static_pointer_cast<BrushUndoMemento>(state);

	_detailFlag = memento._detailFlag;
    appendFaces(memento._faces);

    onFacePlaneChanged();

    for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->DEBUG_verify();
    }
}

/// \brief Appends a copy of \p face to the end of the face list.
FacePtr Brush::addFace(const Face& face) {
    if (m_faces.size() == brush::c_brush_maxFaces) {
        return FacePtr();
    }
    undoSave();
    push_back(FacePtr(new Face(*this, face)));
    onFacePlaneChanged();
    return m_faces.back();
}

/// \brief Appends a new face constructed from the parameters to the end of the face list.
FacePtr Brush::addPlane(const Vector3& p0, const Vector3& p1, const Vector3& p2, const std::string& shader, const TextureProjection& projection) {
    if(m_faces.size() == brush::c_brush_maxFaces) {
        return FacePtr();
    }
    undoSave();
    push_back(FacePtr(new Face(*this, p0, p1, p2, shader, projection)));
    onFacePlaneChanged();
    return m_faces.back();
}

void Brush::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    for (Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i)
    {
        (*i)->setRenderSystem(renderSystem);
    }
}

std::size_t Brush::DEBUG_size() {
    return m_faces.size();
}

Brush::const_iterator Brush::begin() const {
    return m_faces.begin();
}
Brush::const_iterator Brush::end() const {
    return m_faces.end();
}

FacePtr Brush::back() {
    return m_faces.back();
}
const FacePtr Brush::back() const {
    return m_faces.back();
}

void Brush::reserve(std::size_t count) {
    m_faces.reserve(count);
    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->reserve(count);
    }
}

void Brush::push_back(Faces::value_type face) {
    m_faces.push_back(face);

    if (_undoStateSaver)
    {
        m_faces.back()->connectUndoSystem(_undoStateSaver->getUndoSystem());
    }

    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->push_back(*face);
        (*i)->DEBUG_verify();
    }
}

void Brush::pop_back()
{
    if (_undoStateSaver)
    {
        m_faces.back()->disconnectUndoSystem(_undoStateSaver->getUndoSystem());
    }

    m_faces.pop_back();
    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->pop_back();
        (*i)->DEBUG_verify();
    }
}

void Brush::erase(std::size_t index)
{
    if (_undoStateSaver)
    {
        m_faces[index]->disconnectUndoSystem(_undoStateSaver->getUndoSystem());
    }

    m_faces.erase(m_faces.begin() + index);
    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->erase(index);
        (*i)->DEBUG_verify();
    }
}

void Brush::onFacePlaneChanged()
{
    m_planeChanged = true;
    aabbChanged();
}

void Brush::onFaceShaderChanged()
{
    // When the face shader changes, no geometry change is happening
    // therefore no call to onFacePlaneChanged() is necessary

    // Queue an UI update of the texture tools if any of them is listening
	signal_faceShaderChanged().emit();
}

void Brush::onFaceConnectivityChanged()
{
    for (auto i : m_observers)
    {
        i->connectivityChanged();
    }
}

void Brush::onFaceEvaluateTransform()
{
    evaluateTransform();
}

void Brush::clear()
{
    undoSave();
    if (_undoStateSaver)
    {
        forEachFace([&](Face& face) { face.disconnectUndoSystem(_undoStateSaver->getUndoSystem()); });
    }

    m_faces.clear();

    for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->clear();
        (*i)->DEBUG_verify();
    }
}

std::size_t Brush::getNumFaces() const
{
    return m_faces.size();
}

bool Brush::empty() const {
    return m_faces.empty();
}

/// \brief Returns true if any face of the brush contributes to the final B-Rep.
bool Brush::hasContributingFaces() const {
    for (const_iterator i = begin(); i != end(); ++i) {
        if ((*i)->contributes()) {
            return true;
        }
    }
    return false;
}

void Brush::removeEmptyFaces() {
    evaluateBRep();

    std::size_t i = 0;
    while (i < m_faces.size()) {
        if (!m_faces[i]->contributes()) {
            erase(i);
            onFacePlaneChanged();
        }
        else {
            ++i;
        }
    }
}

void Brush::removeRedundantFaces()
{
    for (std::size_t i = 0; i < m_faces.size(); ++i)
    {
        auto& face = *m_faces[i];

        if (face.plane3().isValid() && planeAlreadyDefined(i))
        {
            // Encountering a duplicated plane, this is possible after loading legacy maps
            rWarning() << "Face plane " << face.plane3() << " already defined on this brush, discarding" << std::endl;

            // Remove this face and continue with the same index (decrement to compensate loop increment)
            erase(i--);
        }
    }
}

/// \brief Constructs \p winding from the intersection of \p plane with the other planes of the brush.
void Brush::windingForClipPlane(Winding& winding, const Plane3& plane) const {
    FixedWinding buffer[2];
    bool swap = false;

    // get a poly that covers an effectively infinite area
    buffer[swap].createInfinite(plane, m_maxWorldCoord + 1);

    // chop the poly by all of the other faces
    {
        for (std::size_t i = 0;  i < m_faces.size(); ++i) {
            const Face& clip = *m_faces[i];

            if (clip.plane3() == plane
                || !clip.plane3().isValid() || !plane_unique(i)
                || plane == -clip.plane3())
            {
                continue;
            }

            buffer[!swap].clear();

            {
                // flip the plane, because we want to keep the back side
                Plane3 clipPlane(-clip.plane3().normal(), -clip.plane3().dist());
                buffer[swap].clip(plane, clipPlane, i, buffer[!swap]);
            }

            swap = !swap;
        }
    }

    buffer[swap].writeToWinding(winding);
}

/// \brief Makes this brush a deep-copy of the \p other.
void Brush::copy(const Brush& other)
{
	_detailFlag = other._detailFlag;

    for (Faces::const_iterator i = other.m_faces.begin(); i != other.m_faces.end(); ++i)
	{
        addFace(*(*i));
    }

    onFacePlaneChanged();
}

void Brush::constructCuboid(const AABB& bounds, const std::string& shader)
{
    const unsigned char box[3][2] = { { 0, 1 }, { 2, 0 }, { 1, 2 } };

    Vector3 mins(bounds.origin - bounds.extents);
    Vector3 maxs(bounds.origin + bounds.extents);

    TextureProjection projection;

    clear();
    reserve(6);

    {
        for (int i = 0; i < 3; ++i)
        {
            Vector3 planepts1(maxs);
            Vector3 planepts2(maxs);
            planepts2[box[i][0]] = mins[box[i][0]];
            planepts1[box[i][1]] = mins[box[i][1]];

            addPlane(maxs, planepts1, planepts2, shader, projection);
        }
    }
    {
        for (int i = 0; i < 3; ++i)
        {
            Vector3 planepts1(mins);
            Vector3 planepts2(mins);
            planepts1[box[i][0]] = maxs[box[i][0]];
            planepts2[box[i][1]] = maxs[box[i][1]];

            addPlane(mins, planepts1, planepts2, shader, projection);
        }
    }

    // Passing in the default-constructed projection will result in a very tiny texture scale, fix that
    for (const FacePtr& face : m_faces)
    {
        face->applyDefaultTextureScale();
    }
}

void Brush::constructPrism(const AABB& bounds, std::size_t sides, int axis, const std::string& shader)
{
    TextureProjection projection;

    if (sides < brush::PRISM_MIN_SIDES)
    {
        rError() << "brushPrism: sides " << sides << ": too few sides, minimum is " << brush::PRISM_MIN_SIDES << std::endl;
        return;
    }

    if (sides > brush::PRISM_MAX_SIDES)
    {
        rError() << "brushPrism: sides " << sides << ": too many sides, maximum is " << brush::PRISM_MAX_SIDES << std::endl;
        return;
    }

    clear();
    reserve(sides+2);

    Vector3 mins(bounds.origin - bounds.extents);
    Vector3 maxs(bounds.origin + bounds.extents);

    auto radius = max_extent_2d(bounds.extents, axis);
    const Vector3& mid = bounds.origin;
    Vector3 planepts[3];

    planepts[2][(axis+1)%3] = mins[(axis+1)%3];
    planepts[2][(axis+2)%3] = mins[(axis+2)%3];
    planepts[2][axis] = maxs[axis];
    planepts[1][(axis+1)%3] = maxs[(axis+1)%3];
    planepts[1][(axis+2)%3] = mins[(axis+2)%3];
    planepts[1][axis] = maxs[axis];
    planepts[0][(axis+1)%3] = maxs[(axis+1)%3];
    planepts[0][(axis+2)%3] = maxs[(axis+2)%3];
    planepts[0][axis] = maxs[axis];

    addPlane(planepts[0], planepts[1], planepts[2], shader, projection);

    planepts[0][(axis+1)%3] = mins[(axis+1)%3];
    planepts[0][(axis+2)%3] = mins[(axis+2)%3];
    planepts[0][axis] = mins[axis];
    planepts[1][(axis+1)%3] = maxs[(axis+1)%3];
    planepts[1][(axis+2)%3] = mins[(axis+2)%3];
    planepts[1][axis] = mins[axis];
    planepts[2][(axis+1)%3] = maxs[(axis+1)%3];
    planepts[2][(axis+2)%3] = maxs[(axis+2)%3];
    planepts[2][axis] = mins[axis];

    addPlane(planepts[0], planepts[1], planepts[2], shader, projection);

    for (std::size_t i = 0 ; i < sides ; ++i)
    {
        float sv = sin(i*static_cast<float>(math::PI)*2/sides);
        float cv = cos(i*static_cast<float>(math::PI)*2/sides);

        planepts[0][(axis+1)%3] = floor(mid[(axis+1) % 3] + radius*cv  + 0.5f);
        planepts[0][(axis+2)%3] = floor(mid[(axis+2) % 3] + radius*sv  + 0.5f);
        planepts[0][axis] = mins[axis];

        planepts[1][(axis+1)%3] = planepts[0][(axis+1)%3];
        planepts[1][(axis+2)%3] = planepts[0][(axis+2)%3];
        planepts[1][axis] = maxs[axis];

        planepts[2][(axis+1)%3] = floor(planepts[0][(axis+1)%3] - radius*sv + 0.5f);
        planepts[2][(axis+2)%3] = floor(planepts[0][(axis+2)%3] + radius*cv + 0.5f);
        planepts[2][axis] = maxs[axis];

        addPlane(planepts[0], planepts[1], planepts[2], shader, projection);
    }

    // Passing in the default-constructed projection will result in a very tiny texture scale, fix that
    for (const FacePtr& face : m_faces)
    {
        face->applyDefaultTextureScale();
    }
}

void Brush::constructCone(const AABB& bounds, std::size_t sides, const std::string& shader)
{
    TextureProjection projection;

    if (sides < brush::CONE_MIN_SIDES)
    {
        rError() << "brushCone: sides " << sides << ": too few sides, minimum is " << brush::CONE_MIN_SIDES << std::endl;
        return;
    }

    if (sides > brush::CONE_MAX_SIDES)
    {
        rError() << "brushCone: sides " << sides << ": too many sides, maximum is " << brush::CONE_MAX_SIDES << std::endl;
        return;
    }

    clear();
    reserve(sides+1);

    Vector3 mins(bounds.origin - bounds.extents);
    Vector3 maxs(bounds.origin + bounds.extents);

    auto radius = max_extent(bounds.extents);
    const Vector3& mid = bounds.origin;
    Vector3 planepts[3];

    planepts[0][0] = mins[0];planepts[0][1] = mins[1];planepts[0][2] = mins[2];
    planepts[1][0] = maxs[0];planepts[1][1] = mins[1];planepts[1][2] = mins[2];
    planepts[2][0] = maxs[0];planepts[2][1] = maxs[1];planepts[2][2] = mins[2];

    addPlane(planepts[0], planepts[1], planepts[2], shader, projection);

    for (std::size_t i = 0 ; i < sides ; ++i)
    {
        float sv = sin (i*static_cast<float>(math::PI)*2/sides);
        float cv = cos (i*static_cast<float>(math::PI)*2/sides);

        planepts[0][0] = floor(mid[0] + radius*cv + 0.5f);
        planepts[0][1] = floor(mid[1] + radius*sv + 0.5f);
        planepts[0][2] = mins[2];

        planepts[1][0] = mid[0];
        planepts[1][1] = mid[1];
        planepts[1][2] = maxs[2];

        planepts[2][0] = floor(planepts[0][0] - radius*sv + 0.5f);
        planepts[2][1] = floor(planepts[0][1] + radius*cv + 0.5f);
        planepts[2][2] = maxs[2];

        addPlane(planepts[0], planepts[1], planepts[2], shader, projection);
    }

    // Passing in the default-constructed projection will result in a very tiny texture scale, fix that
    for (const FacePtr& face : m_faces)
    {
        face->applyDefaultTextureScale();
    }
}

void Brush::constructSphere(const AABB& bounds, std::size_t sides, const std::string& shader)
{
    TextureProjection projection;

    if (sides < brush::SPHERE_MIN_SIDES)
    {
        rError() << "brushSphere: sides " << sides << ": too few sides, minimum is " << brush::SPHERE_MIN_SIDES << std::endl;
        return;
    }
    if (sides > brush::SPHERE_MAX_SIDES)
    {
        rError() << "brushSphere: sides " << sides << ": too many sides, maximum is " << brush::SPHERE_MAX_SIDES << std::endl;
        return;
    }

    clear();
    reserve(sides*sides);

    auto radius = max_extent(bounds.extents);
    const Vector3& mid = bounds.origin;
    Vector3 planepts[3];

    float dt = 2 * static_cast<float>(math::PI) / sides;
    float dp = static_cast<float>(math::PI) / sides;

    for (std::size_t i = 0; i < sides; i++)
    {
        for (std::size_t j = 0; j < sides - 1; j++)
        {
            float t = i * dt;
            float p = static_cast<float>(j * dp - math::PI / 2);

            planepts[0] = mid + Vector3::createForSpherical(t, p)*radius;
            planepts[1] = mid + Vector3::createForSpherical(t, p + dp)*radius;
            planepts[2] = mid + Vector3::createForSpherical(t + dt, p + dp)*radius;

            addPlane(planepts[0], planepts[1], planepts[2], shader, projection);
        }
    }

    {
        float p = (sides - 1) * dp - static_cast<float>(math::PI) / 2;

        for (std::size_t i = 0; i < sides; i++)
        {
            float t = i * dt;

            planepts[0] = mid + Vector3::createForSpherical(t, p)*radius;
            planepts[1] = mid + Vector3::createForSpherical(t + dt, p + dp)*radius;
            planepts[2] = mid + Vector3::createForSpherical(t + dt, p)*radius;

            addPlane(planepts[0], planepts[1], planepts[2], shader, projection);
        }
    }

    // Passing in the default-constructed projection will result in a very tiny texture scale, fix that
    for (const FacePtr& face : m_faces)
    {
        face->applyDefaultTextureScale();
    }
}

// greebo: this code is modeled after http://geomalgorithms.com/a13-_intersect-4.html
bool Brush::getIntersection(const Ray& ray, Vector3& intersection)
{
	double tEnter = 0;		// maximum entering segment parameter
	double tLeave = 5000;	// minimum leaving segment parameter (let's assume 5000 units for now)

	Vector3 direction = ray.direction.getNormalised(); // normalise the ray direction

	for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i)
	{
		const Face& face = *(*i);

		if (!face.contributes()) continue; // skip non-contributing faces

		auto n = -(ray.origin - face.getWinding().front().vertex).dot(face.getPlane3().normal());
        auto d = direction.dot(face.getPlane3().normal());

		if (d == 0) // is the ray parallel to the face?
		{
			if (n < 0)
			{
				return false; // since the ray cannot intersect the brush;
			}
			else
			{
				// the ray cannot enter or leave the brush across this face
				continue;
			}
		}

		auto t = n / d;

		if (d < 0)
		{
			// ray is entering the brush across this face
			tEnter = std::max(tEnter, t);

			if (tEnter > tLeave)
			{
				return false; // the ray enters the brush after leaving => cannot intersect
			}
		}
		else if (d > 0)
		{
			// ray is leaving the brush across this face
			tLeave = std::min(tLeave, t);

			if (tLeave < tEnter)
			{
				return false; // the ray leaves the brush before entering => cannot intersect
			}
		}
	}

	assert(tEnter <= tLeave);
	intersection = ray.origin + direction * tEnter;

	return true;
}

sigc::signal<void>& Brush::signal_faceShaderChanged()
{
	static sigc::signal<void> _sigFaceShaderChanged;
	return _sigFaceShaderChanged;
}

void Brush::edge_push_back(FaceVertexId faceVertex) {
    m_select_edges.push_back(SelectableEdge(m_faces, faceVertex));
    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->edge_push_back(m_select_edges.back());
    }
}

void Brush::edge_clear() {
    m_select_edges.clear();
    for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->edge_clear();
    }
}

void Brush::vertex_push_back(FaceVertexId faceVertex) {
    m_select_vertices.push_back(SelectableVertex(m_faces, faceVertex));
    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->vertex_push_back(m_select_vertices.back());
    }
}

void Brush::vertex_clear() {
    m_select_vertices.clear();
    for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
        (*i)->vertex_clear();
    }
}

/// \brief Returns true if the face identified by \p index is preceded by another plane that takes priority over it.
bool Brush::plane_unique(std::size_t index) const {
    // duplicate plane
    for (std::size_t i = 0; i < m_faces.size(); ++i) {
        if (index != i && !plane3_inside(m_faces[index]->plane3(), m_faces[i]->plane3())) {
            return false;
        }
    }
    return true;
}

bool Brush::planeAlreadyDefined(std::size_t index) const
{
    for (std::size_t i = 0; index < m_faces.size() && i < index; ++i)
    {
        if (!plane3_inside(m_faces[index]->plane3(), m_faces[i]->plane3()))
        {
            return true;
        }
    }

    return false;
}

/// \brief Removes edges that are smaller than the tolerance used when generating brush windings.
void Brush::removeDegenerateEdges() {
    for (std::size_t i = 0;  i < m_faces.size(); ++i) {
        Winding& winding = m_faces[i]->getWinding();

        for (std::size_t index = 0; index < winding.size();) {
            //std::size_t index = std::distance(winding.begin(), j);
            std::size_t next = winding.next(index);

            if (Edge_isDegenerate(winding[index].vertex, winding[next].vertex)) {
                Winding& other = m_faces[winding[index].adjacent]->getWinding();
                std::size_t adjacent = other.findAdjacent(i);
                if (adjacent != brush::c_brush_maxFaces) {
                    other.erase(other.begin() + adjacent);
                }

                // Delete and leave index where it is
                winding.erase(winding.begin() + index);
            }
            else {
                ++index;
            }
        }
    }
}

/// \brief Invalidates faces that have only two vertices in their winding, while preserving edge-connectivity information.
void Brush::removeDegenerateFaces() {
    // save adjacency info for degenerate faces
    for (std::size_t i = 0;  i < m_faces.size(); ++i) {
        Winding& degen = m_faces[i]->getWinding();

        if (degen.size() == 2) {
            /*rConsole() << "Removed degenerate face: " << Vector3(m_faces[i]->getPlane().plane3().normal())
                                << " - " << float(m_faces[i]->getPlane().plane3().dist()) << std::endl;*/

            // this is an "edge" face, where the plane touches the edge of the brush
            {
                Winding& winding = m_faces[degen[0].adjacent]->getWinding();
                std::size_t index = winding.findAdjacent(i);
                if (index != brush::c_brush_maxFaces) {
                    winding[index].adjacent = degen[1].adjacent;
                }
            }

            {
                Winding& winding = m_faces[degen[1].adjacent]->getWinding();
                std::size_t index = winding.findAdjacent(i);

                if (index != brush::c_brush_maxFaces) {
                    winding[index].adjacent = degen[0].adjacent;
                }
            }

            degen.resize(0);
        }
    }
}

/// \brief Removes edges that have the same adjacent-face as their immediate neighbour.
void Brush::removeDuplicateEdges() {
    // verify face connectivity graph
    for(std::size_t i = 0; i < m_faces.size(); ++i) {
        //if(m_faces[i]->contributes())
            {
                Winding& winding = m_faces[i]->getWinding();
                for (std::size_t j = 0; j != winding.size();) {
                    std::size_t next = winding.next(j);
                    if (winding[j].adjacent == winding[next].adjacent) {
                        winding.erase(winding.begin() + next);
                    }
                    else {
                        ++j;
                    }
                }
            }
        }
    }

/// \brief Removes edges that do not have a matching pair in their adjacent-face.
void Brush::verifyConnectivityGraph() {
    // verify face connectivity graph
    for (std::size_t i = 0; i < m_faces.size(); ++i) {
        //if(m_faces[i]->contributes())
        {
            Winding& winding = m_faces[i]->getWinding();

            for (std::size_t j = 0; j < winding.size();) {
                WindingVertex& vertex = winding[j];

                // remove unidirectional graph edges
                if (vertex.adjacent == brush::c_brush_maxFaces
                    || m_faces[vertex.adjacent]->getWinding().findAdjacent(i) == brush::c_brush_maxFaces)
                {
                    // Delete the offending vertex and leave the index j where it is
                    winding.erase(winding.begin() + j);
                }
                else {
                    ++j;
                }
            }

            /*for (Winding::iterator j = winding.begin(); j != winding.end();) {
                // remove unidirectional graph edges
                if (j->adjacent == c_brush_maxFaces
                    || m_faces[j->adjacent]->getWinding().findAdjacent(i) == c_brush_maxFaces)
                {
                    // Delete and return the new iterator
                    j = winding.erase(j);
                }
                else {
                    ++j;
                }
            }*/
        }
    }
}

/// \brief Returns true if the brush is a finite volume. A brush without a finite volume extends past the maximum world bounds and is not valid.
bool Brush::isBounded() {
    for (const_iterator i = begin(); i != end(); ++i) {
        if (!(*i)->is_bounded()) {
            return false;
        }
    }
    return true;
}

bool Brush::buildWindings()
{
    m_aabb_local = AABB();

    for (std::size_t i = 0;  i < m_faces.size(); ++i)
    {
        auto& face = *m_faces[i];

        if (!face.plane3().isValid() || !plane_unique(i))
        {
            face.getWinding().resize(0);
        }
        else
        {
            windingForClipPlane(face.getWinding(), face.plane3());

            // update brush bounds
            const auto& winding = face.getWinding();

            for (const auto& vertex : winding)
			{
                m_aabb_local.includePoint(vertex.vertex);
            }

            // update texture coordinates
            face.emitTextureCoordinates();
        }

        // greebo: Update the winding, now that it's constructed
        face.updateWinding();
    }

    bool degenerate = !isBounded();

    if (!degenerate)
    {
        // clean up connectivity information.
        // these cleanups must be applied in a specific order.
        removeDegenerateEdges();
        removeDegenerateFaces();
        removeDuplicateEdges();
        verifyConnectivityGraph();
    }

    return degenerate;
}

struct SListNode {
    SListNode* m_next;
};

class ProximalVertex {
public:
    const SListNode* m_vertices;

    ProximalVertex(const SListNode* next)
        : m_vertices(next)
    {}

    bool operator<(const ProximalVertex& other) const {
        if (!(operator==(other))) {
            return m_vertices < other.m_vertices;
        }
        return false;
    }

    bool operator==(const ProximalVertex& other) const {
        const SListNode* v = m_vertices;
        std::size_t DEBUG_LOOP = 0;

        do {
            if (v == other.m_vertices)
                return true;
            v = v->m_next;
            //ASSERT_MESSAGE(DEBUG_LOOP < c_brush_maxFaces, "infinite loop");
            if (!(DEBUG_LOOP < brush::c_brush_maxFaces)) {
                break;
            }
            ++DEBUG_LOOP;
        } while(v != m_vertices);

        return false;
    }
};

typedef std::vector<SListNode> ProximalVertexArray;
std::size_t ProximalVertexArray_index(const ProximalVertexArray& array, const ProximalVertex& vertex) {
    return vertex.m_vertices - &array.front();
}

/// \brief Constructs the face windings and updates anything that depends on them.
void Brush::buildBRep() {
  bool degenerate = buildWindings();

  static const Vector3& colourVertexVec = GlobalBrush().getSettings().getVertexColour();
  const Colour4b colour_vertex(int(colourVertexVec[0]*255), int(colourVertexVec[1]*255),
                                   int(colourVertexVec[2]*255), 255);

  std::size_t faces_size = 0;
  std::size_t faceVerticesCount = 0;
  for (Faces::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
    if ((*i)->contributes()) {
      ++faces_size;
    }
    faceVerticesCount += (*i)->getWinding().size();
  }

  if(degenerate || faces_size < 4 || faceVerticesCount != (faceVerticesCount>>1)<<1) // sum of vertices for each face of a valid polyhedron is always even
  {
    _uniqueVertexPoints.resize(0);

    vertex_clear();
    edge_clear();

    _edgeIndices.resize(0);
    _edgeFaces.resize(0);

    _faceCentroidPoints.resize(0);
    _uniqueEdgePoints.resize(0);

    for(Faces::iterator i = m_faces.begin(); i != m_faces.end(); ++i)
    {
      (*i)->getWinding().resize(0);
    }
  }
  else
  {
    {
      typedef std::vector<FaceVertexId> FaceVertices;
      FaceVertices faceVertices;
      faceVertices.reserve(faceVerticesCount);

      {
        for(std::size_t i = 0; i != m_faces.size(); ++i)
        {
          for(std::size_t j = 0; j < m_faces[i]->getWinding().size(); ++j)
          {
            faceVertices.push_back(FaceVertexId(i, j));
          }
        }
      }

      IndexBuffer uniqueEdgeIndices;
      typedef std::vector<ProximalVertex> UniqueEdges;
      UniqueEdges uniqueEdges;

      uniqueEdgeIndices.reserve(faceVertices.size());
      uniqueEdges.reserve(faceVertices.size());

      {
        ProximalVertexArray edgePairs;
        edgePairs.resize(faceVertices.size());

        {
          for(std::size_t i=0; i<faceVertices.size(); ++i)
          {
            edgePairs[i].m_next = &edgePairs.front() + absoluteIndex(next_edge(m_faces, faceVertices[i]));
          }
        }

        {
          UniqueVertexBuffer<ProximalVertex> inserter(uniqueEdges);
          for(ProximalVertexArray::iterator i = edgePairs.begin(); i != edgePairs.end(); ++i)
          {
            uniqueEdgeIndices.push_back(inserter.insert(ProximalVertex(&(*i))));
          }
        }

        {
          edge_clear();
          m_select_edges.reserve(uniqueEdges.size());
          for(UniqueEdges::iterator i = uniqueEdges.begin(); i != uniqueEdges.end(); ++i)
          {
            edge_push_back(faceVertices[ProximalVertexArray_index(edgePairs, *i)]);
          }
        }

        {
          _edgeFaces.resize(uniqueEdges.size());
          for(std::size_t i=0; i<uniqueEdges.size(); ++i)
          {
            FaceVertexId faceVertex = faceVertices[ProximalVertexArray_index(edgePairs, uniqueEdges[i])];
            _edgeFaces[i] = EdgeFaces(faceVertex.getFace(), m_faces[faceVertex.getFace()]->getWinding()[faceVertex.getVertex()].adjacent);
          }
        }

        {
          _uniqueEdgePoints.resize(uniqueEdges.size());

          for(std::size_t i=0; i<uniqueEdges.size(); ++i)
          {
            FaceVertexId faceVertex = faceVertices[ProximalVertexArray_index(edgePairs, uniqueEdges[i])];

            const Winding& w = m_faces[faceVertex.getFace()]->getWinding();
            Vector3 edge = math::midPoint(w[faceVertex.getVertex()].vertex,
                                          w[w.next(faceVertex.getVertex())].vertex);
            _uniqueEdgePoints[i] = edge;
          }
        }

      }


      IndexBuffer uniqueVertexIndices;
      typedef std::vector<ProximalVertex> UniqueVertices;
      UniqueVertices uniqueVertices;

      uniqueVertexIndices.reserve(faceVertices.size());
      uniqueVertices.reserve(faceVertices.size());

      {
        ProximalVertexArray vertexRings;
        vertexRings.resize(faceVertices.size());

        {
          for(std::size_t i=0; i<faceVertices.size(); ++i)
          {
            vertexRings[i].m_next = &vertexRings.front() + absoluteIndex(next_vertex(m_faces, faceVertices[i]));
          }
        }

        {
          UniqueVertexBuffer<ProximalVertex> inserter(uniqueVertices);
          for(ProximalVertexArray::iterator i = vertexRings.begin(); i != vertexRings.end(); ++i)
          {
            uniqueVertexIndices.push_back(inserter.insert(ProximalVertex(&(*i))));
          }
        }

        {
          vertex_clear();
          m_select_vertices.reserve(uniqueVertices.size());
          for(UniqueVertices::iterator i = uniqueVertices.begin(); i != uniqueVertices.end(); ++i)
          {
            vertex_push_back(faceVertices[ProximalVertexArray_index(vertexRings, (*i))]);
          }
        }

        {
          _uniqueVertexPoints.resize(uniqueVertices.size());

          for(std::size_t i=0; i<uniqueVertices.size(); ++i)
          {
            FaceVertexId faceVertex = faceVertices[ProximalVertexArray_index(vertexRings, uniqueVertices[i])];

            const Winding& winding = m_faces[faceVertex.getFace()]->getWinding();
            _uniqueVertexPoints[i] = winding[faceVertex.getVertex()].vertex;
          }
        }
      }

      if((uniqueVertices.size() + faces_size) - uniqueEdges.size() != 2)
      {
        rError() << "Final B-Rep: inconsistent vertex count\n";
      }

      // edge-index list for wireframe rendering
      {
        _edgeIndices.resize(uniqueEdgeIndices.size());

        for(std::size_t i=0, count=0; i<m_faces.size(); ++i)
        {
          const Winding& winding = m_faces[i]->getWinding();
          for(std::size_t j = 0; j < winding.size(); ++j)
          {
            const RenderIndex edge_index = uniqueEdgeIndices[count+j];

            _edgeIndices[edge_index].first = uniqueVertexIndices[count + j];
            _edgeIndices[edge_index].second = uniqueVertexIndices[count + winding.next(j)];
          }
          count += winding.size();
        }
      }
    }

    {
      _faceCentroidPoints.resize(m_faces.size());

      for(std::size_t i=0; i<m_faces.size(); ++i)
      {
        m_faces[i]->construct_centroid();
        _faceCentroidPoints[i] = m_faces[i]->centroid();
      }
    }
  }
}

const std::vector<Vector3>& Brush::getVertices(selection::ComponentSelectionMode mode) const
{
    static std::vector<Vector3> _emptyVertexList;

    switch (mode)
    {
        case selection::ComponentSelectionMode::Vertex: 
            return _uniqueVertexPoints;

        case selection::ComponentSelectionMode::Edge: 
            return _uniqueEdgePoints;

        case selection::ComponentSelectionMode::Face: 
            return _faceCentroidPoints;

        default:
            // We're getting here if single brush faces have been selected using Ctrl-Shift-LMB
            // and the RenderableBrushVertices is asking for visible components
            return _emptyVertexList;
    }
}

// ----------------------------------------------------------------------------

double Brush::m_maxWorldCoord = 0;

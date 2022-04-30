#include "BrushNode.h"

#include "ivolumetest.h"
#include "ifilter.h"
#include "iradiant.h"
#include "icounter.h"
#include "iclipper.h"
#include "imap.h"
#include "ientity.h"
#include "math/Frustum.h"
#include "math/Hash.h"
#include <functional>

// Constructor
BrushNode::BrushNode() :
	scene::SelectableNode(),
	m_brush(*this),
	_renderableComponentsNeedUpdate(true),
    _numSelectedComponents(0),
    _untransformedOriginChanged(true),
    _renderableVertices(m_brush, _selectedPoints),
    _facesNeedRenderableUpdate(true)
{
	m_brush.attach(*this); // BrushObserver

    // Try to anticipate a few face additions to avoid reallocations during map parsing
    reserve(6);
}

// Copy Constructor
BrushNode::BrushNode(const BrushNode& other) :
	scene::SelectableNode(other),
	scene::Cloneable(other),
	Snappable(other),
	IBrushNode(other),
	BrushObserver(other),
	SelectionTestable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	PlaneSelectable(other),
	Transformable(other),
	m_brush(*this, other.m_brush),
	_renderableComponentsNeedUpdate(true),
    _numSelectedComponents(0),
    _untransformedOriginChanged(true),
    _renderableVertices(m_brush, _selectedPoints),
    _facesNeedRenderableUpdate(true)
{
	m_brush.attach(*this); // BrushObserver
}

BrushNode::~BrushNode()
{
	m_brush.detach(*this); // BrushObserver
}

scene::INode::Type BrushNode::getNodeType() const
{
	return Type::Brush;
}

const AABB& BrushNode::localAABB() const {
	return m_brush.localAABB();
}

std::string BrushNode::getFingerprint()
{
    constexpr std::size_t SignificantDigits = scene::SignificantFingerprintDoubleDigits;

    if (m_brush.getNumFaces() == 0)
    {
        return std::string(); // empty brushes produce an empty fingerprint
    }

    math::Hash hash;

    hash.addSizet(static_cast<std::size_t>(m_brush.getDetailFlag() + 1));

    hash.addSizet(m_brush.getNumFaces());

    // Combine all face plane equations
    for (const auto& face : m_brush)
    {
        // Plane equation
        hash.addVector3(face->getPlane3().normal(), SignificantDigits);
        hash.addDouble(face->getPlane3().dist(), SignificantDigits);

        // Material Name
        hash.addString(face->getShader());

        // Texture Matrix
        auto texdef = face->getProjectionMatrix();
        hash.addDouble(texdef.xx(), SignificantDigits);
        hash.addDouble(texdef.yx(), SignificantDigits);
        hash.addDouble(texdef.zx(), SignificantDigits);
        hash.addDouble(texdef.xy(), SignificantDigits);
        hash.addDouble(texdef.yy(), SignificantDigits);
        hash.addDouble(texdef.zy(), SignificantDigits);
    }

    return hash;
}

// Snappable implementation
void BrushNode::snapto(float snap) {
	m_brush.snapto(snap);
}

void BrushNode::snapComponents(float snap) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->snapComponents(snap);
	}
}

void BrushNode::testSelect(Selector& selector, SelectionTest& test)
{
    // BeginMesh(true): Always treat brush faces twosided when in orthoview
	test.BeginMesh(localToWorld(), !test.getVolume().fill());

	SelectionIntersection best;
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
	{
		if (i->faceIsVisible())
		{
			i->testSelect(test, best);
		}
	}

	if (best.isValid()) {
		selector.addIntersection(best);
	}
}

inline bool checkFaceInstancesForSelectedComponents(const FaceInstances& instances)
{
    for (const auto& face :  instances)
    {
        if (face.selectedComponents())
        {
            return true;
        }
    }

    return false;
}

bool BrushNode::isSelectedComponents() const
{
    // Debug builds check whether the _numSelectedComponents counter is providing the
    // same result as the old code it replaced below.
    assert(_numSelectedComponents > 0 == checkFaceInstancesForSelectedComponents(m_faceInstances));

    return _numSelectedComponents > 0;
#if 0
	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if (i->selectedComponents()) {
			return true;
		}
	}
	return false;
#endif
}

void BrushNode::setSelectedComponents(bool select, selection::ComponentSelectionMode mode)
{
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->setSelected(mode, select);
	}
}

void BrushNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	// Component mode, invert the component selection
	switch (mode)
	{
	case selection::ComponentSelectionMode::Vertex:
		for (brush::VertexInstance& vertex : m_vertexInstances)
		{
			vertex.invertSelected();
		}
		break;
	case selection::ComponentSelectionMode::Edge:
		for (EdgeInstance& edge : m_edgeInstances)
		{
			edge.invertSelected();
		}
		break;
	case selection::ComponentSelectionMode::Face:
		for (FaceInstance& face : m_faceInstances)
		{
			face.invertSelected();
		}
		break;
	case selection::ComponentSelectionMode::Default:
		break;
	} // switch
}

void BrushNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	test.BeginMesh(localToWorld());

	switch (mode) {
		case selection::ComponentSelectionMode::Vertex: {
				for (VertexInstances::iterator i = m_vertexInstances.begin(); i != m_vertexInstances.end(); ++i) {
					i->testSelect(selector, test);
				}
			}
		break;
		case selection::ComponentSelectionMode::Edge: {
				for (EdgeInstances::iterator i = m_edgeInstances.begin(); i != m_edgeInstances.end(); ++i) {
					i->testSelect(selector, test);
				}
			}
			break;
		case selection::ComponentSelectionMode::Face: {
				if (test.getVolume().fill()) {
					for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
						i->testSelect(selector, test);
					}
				}
				else {
					for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
						i->testSelect_centroid(selector, test);
					}
				}
			}
			break;
		default:
			break;
	}
}

const AABB& BrushNode::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();

	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->iterate_selected(m_aabb_component);
	}

	return m_aabb_component;
}

void BrushNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());

	PlanePointer brushPlanes[brush::c_brush_maxFaces];
	PlanesIterator j = brushPlanes;

	for (Brush::const_iterator i = m_brush.begin(); i != m_brush.end(); ++i) {
		*j++ = &(*i)->plane3();
	}

	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->selectPlane(selector, Line(test.getNear(), test.getFar()), brushPlanes, j, selectedPlaneCallback);
	}
}

void BrushNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->selectReversedPlane(selector, selectedPlanes);
	}
}

void BrushNode::selectedChangedComponent(const ISelectable& selectable)
{
	_renderableComponentsNeedUpdate = true;

    if (selectable.isSelected())
    {
        ++_numSelectedComponents;
    }
    else
    {
        assert(_numSelectedComponents > 0);
        --_numSelectedComponents;
    }

	GlobalSelectionSystem().onComponentSelection(SelectableNode::getSelf(), selectable);
}

// IBrushNode implementation
Brush& BrushNode::getBrush() {
	return m_brush;
}

IBrush& BrushNode::getIBrush() {
	return m_brush;
}

void BrushNode::translate(const Vector3& translation)
{
	m_brush.translate(translation);
}

scene::INodePtr BrushNode::clone() const {
	return std::make_shared<BrushNode>(*this);
}

void BrushNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    m_brush.connectUndoSystem(root.getUndoSystem());
	GlobalCounters().getCounter(counterBrushes).increment();

    // Update the origin information needed for transformations
    _untransformedOriginChanged = true;
    _renderableVertices.queueUpdate();

	SelectableNode::onInsertIntoScene(root);
}

void BrushNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	// De-select this node
	setSelected(false);

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);
	setSelectedComponents(false, selection::ComponentSelectionMode::Edge);
	setSelectedComponents(false, selection::ComponentSelectionMode::Face);

	GlobalCounters().getCounter(counterBrushes).decrement();
    m_brush.disconnectUndoSystem(root.getUndoSystem());
    _renderableVertices.clear();

	SelectableNode::onRemoveFromScene(root);
}

void BrushNode::clear() {
	m_faceInstances.clear();
}

void BrushNode::reserve(std::size_t size) {
	m_faceInstances.reserve(size);
}

void BrushNode::push_back(Face& face)
{
	m_faceInstances.emplace_back(face, std::bind(&BrushNode::selectedChangedComponent, this, std::placeholders::_1));
    _untransformedOriginChanged = true;
}

void BrushNode::pop_back() {
	ASSERT_MESSAGE(!m_faceInstances.empty(), "erasing invalid element");
	m_faceInstances.pop_back();
    _untransformedOriginChanged = true;
}

void BrushNode::erase(std::size_t index) {
	ASSERT_MESSAGE(index < m_faceInstances.size(), "erasing invalid element");
	m_faceInstances.erase(m_faceInstances.begin() + index);
}
void BrushNode::connectivityChanged() {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->connectivityChanged();
	}
}

void BrushNode::edge_clear() {
	m_edgeInstances.clear();
}
void BrushNode::edge_push_back(SelectableEdge& edge) {
	m_edgeInstances.push_back(EdgeInstance(m_faceInstances, edge));
}

void BrushNode::vertex_clear() {
	m_vertexInstances.clear();
}
void BrushNode::vertex_push_back(SelectableVertex& vertex) {
	m_vertexInstances.push_back(brush::VertexInstance(m_faceInstances, vertex));
}

void BrushNode::DEBUG_verify() {
	ASSERT_MESSAGE(m_faceInstances.size() == m_brush.DEBUG_size(), "FATAL: mismatch");
}

void BrushNode::onFaceNeedsRenderableUpdate()
{
    _facesNeedRenderableUpdate = true;
}

void BrushNode::onPreRender(const VolumeTest& volume)
{
    m_brush.evaluateBRep();

    assert(_renderEntity);

    // Run the face updates only if requested
    if (_facesNeedRenderableUpdate)
    {
        _facesNeedRenderableUpdate = false;

        // Every face is asked to run the rendering preparations
        // to link/unlink their geometry to/from the active shader
        for (auto& faceInstance : m_faceInstances)
        {
            auto& face = faceInstance.getFace();

            face.getWindingSurfaceSolid().update(face.getFaceShader().getGLShader(), *_renderEntity);
            face.getWindingSurfaceWireframe().update(_renderEntity->getWireShader(), *_renderEntity);
        }
    }

    if (isSelected() && GlobalSelectionSystem().Mode() == selection::SelectionSystem::eComponent)
    {
        updateSelectedPointsArray();

        _renderableVertices.setComponentMode(GlobalSelectionSystem().ComponentMode());
        _renderableVertices.update(_pointShader);
    }
    else
    {
        _renderableVertices.clear();
        _renderableVertices.queueUpdate();
    }
}

void BrushNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    // Check for the override status of this brush
    bool forceVisible = isForcedVisible();
    bool wholeBrushSelected = isSelected() || Node_isSelected(getParent());

    collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, wholeBrushSelected);

    // Submit the renderable geometry for each face
    for (auto& faceInstance : m_faceInstances)
    {
        // Skip invisible faces before traversing further
        if (!forceVisible && !faceInstance.faceIsVisible()) continue;

        Face& face = faceInstance.getFace();
        if (face.intersectVolume(volume))
        {
            bool highlight = wholeBrushSelected || faceInstance.selectedComponents();

            if (!highlight) continue;

            collector.setHighlightFlag(IRenderableCollector::Highlight::Faces, true);

            // Submit the RenderableWinding as reference, it will render the winding in polygon mode
            collector.addHighlightRenderable(volume.fill() ? 
                face.getWindingSurfaceSolid() : face.getWindingSurfaceWireframe(), Matrix4::getIdentity());

            collector.setHighlightFlag(IRenderableCollector::Highlight::Faces, false);
        }
    }

    if (wholeBrushSelected && GlobalClipper().clipMode())
    {
        collector.addHighlightRenderable(m_clipPlane, Matrix4::getIdentity());
    }

    collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, false);
}

void BrushNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	if (renderSystem)
	{
        _pointShader = renderSystem->capture(BuiltInShaderType::BigPoint);
        _renderableVertices.queueUpdate();
	}
	else
	{
        _pointShader.reset();
        _renderableVertices.clear();
	}

	m_brush.setRenderSystem(renderSystem);
	m_clipPlane.setRenderSystem(renderSystem);
}

std::size_t BrushNode::getHighlightFlags()
{
	if (!isSelected() && !isSelectedComponents()) return Highlight::NoHighlight;

	return isGroupMember() ? (Highlight::Selected | Highlight::GroupMember) : Highlight::Selected;
}

void BrushNode::updateSelectedPointsArray()
{
    if (!_renderableComponentsNeedUpdate) return;

    _renderableComponentsNeedUpdate = false;

    _selectedPoints.clear();

    for (const auto& faceInstance : m_faceInstances)
    {
        if (faceInstance.getFace().contributes())
        {
            faceInstance.SelectedComponents_foreach([&](const Vector3& vertex)
            {
                _selectedPoints.push_back(vertex);
            });
        }
    }

    _renderableVertices.queueUpdate();
}

void BrushNode::evaluateTransform()
{
    if (getTransformationType() == NoTransform)
    {
        return;
    }

	if (getType() == TRANSFORM_PRIMITIVE)
    {
        // If this is a pure translation (no other bits set), call the specialised method
        if (getTransformationType() == Translation)
        {
            for (auto face : m_brush)
            {
                face->translate(getTranslation());
            }
        }
        else
        {
            auto transform = calculateTransform();

            if (transform != Matrix4::getIdentity())
            {
                m_brush.transform(transform);
            }
        }
	}
	else
    {
		transformComponents(calculateTransform());
	}
}

bool BrushNode::getIntersection(const Ray& ray, Vector3& intersection)
{
	return m_brush.getIntersection(ray, intersection);
}

void BrushNode::updateFaceVisibility()
{
	// Trigger an update, the brush might not have any faces calculated so far
	m_brush.evaluateBRep();

	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
	{
		i->updateFaceVisibility();
	}
}

void BrushNode::transformComponents(const Matrix4& matrix) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->transformComponents(matrix);
	}
}

void BrushNode::setClipPlane(const Plane3& plane)
{
    if (_renderEntity)
    {
        m_clipPlane.setPlane(m_brush, plane, *_renderEntity);
    }
}

void BrushNode::forEachFaceInstance(const std::function<void(FaceInstance&)>& functor)
{
	std::for_each(m_faceInstances.begin(), m_faceInstances.end(), functor);
}

const Vector3& BrushNode::getUntransformedOrigin()
{
    if (_untransformedOriginChanged)
    {
        _untransformedOriginChanged = false;
        _untransformedOrigin = worldAABB().getOrigin();
    }

    return _untransformedOrigin;
}

bool BrushNode::facesAreForcedVisible()
{
    return isForcedVisible();
}

void BrushNode::onPostUndo()
{
    // The windings are usually lazy-evaluated when some code
    // is calling localAABB() during rendering.
    // To avoid the texture tool from rendering old texture coords
    // We evaluate the windings right after undo
    m_brush.evaluateBRep();
}

void BrushNode::onPostRedo()
{
    m_brush.evaluateBRep();
}

void BrushNode::_onTransformationChanged()
{
    m_brush.transformChanged();

    _renderableVertices.queueUpdate();
	_renderableComponentsNeedUpdate = true;
}

void BrushNode::_applyTransformation()
{
	m_brush.revertTransform();
	evaluateTransform();
	m_brush.freezeTransform();

    _untransformedOriginChanged = true;
}

void BrushNode::onVisibilityChanged(bool isVisibleNow)
{
    SelectableNode::onVisibilityChanged(isVisibleNow);

    // Let each face know about the change
    forEachFaceInstance([=](FaceInstance& face)
    {
        face.getFace().onBrushVisibilityChanged(isVisibleNow);
    });

    m_clipPlane.clear();
    _renderableVertices.clear();
}

void BrushNode::onSelectionStatusChange(bool changeGroupStatus)
{
    SelectableNode::onSelectionStatusChange(changeGroupStatus);

    // In clip mode we need to check if there's an active clip plane defined in the scene
    if (isSelected() && GlobalClipper().clipMode())
    {
        setClipPlane(GlobalClipper().getClipPlane());
    }
    else
    {
        m_clipPlane.clear();
    }
}

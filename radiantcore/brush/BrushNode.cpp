#include "BrushNode.h"

#include "ivolumetest.h"
#include "icounter.h"
#include "iclipper.h"
#include "imap.h"
#include "math/Hash.h"
#include <functional>

BrushNode::BrushNode() :
	scene::SelectableNode(),
	_brush(*this),
	_renderableComponentsNeedUpdate(true),
    _numSelectedComponents(0),
    _untransformedOriginChanged(true),
    _renderableVertices(_brush, _selectedPoints),
    _facesNeedRenderableUpdate(true)
{
	_brush.attach(*this); // BrushObserver

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
	_brush(*this, other._brush),
	_renderableComponentsNeedUpdate(true),
    _numSelectedComponents(0),
    _untransformedOriginChanged(true),
    _renderableVertices(_brush, _selectedPoints),
    _facesNeedRenderableUpdate(true)
{
	_brush.attach(*this); // BrushObserver
}

BrushNode::~BrushNode()
{
	_brush.detach(*this); // BrushObserver
}

scene::INode::Type BrushNode::getNodeType() const
{
	return Type::Brush;
}

const AABB& BrushNode::localAABB() const {
	return _brush.localAABB();
}

std::string BrushNode::getFingerprint()
{
    constexpr std::size_t SignificantDigits = scene::SignificantFingerprintDoubleDigits;

    if (_brush.getNumFaces() == 0)
    {
        return std::string(); // empty brushes produce an empty fingerprint
    }

    math::Hash hash;

    hash.addSizet(static_cast<std::size_t>(_brush.getDetailFlag() + 1));

    hash.addSizet(_brush.getNumFaces());

    // Combine all face plane equations
    for (const auto& face : _brush)
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
	_brush.snapto(snap);
}

void BrushNode::snapComponents(float snap) {
	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
		i->snapComponents(snap);
	}
}

void BrushNode::testSelect(Selector& selector, SelectionTest& test)
{
    // BeginMesh(true): Always treat brush faces twosided when in orthoview
	test.BeginMesh(localToWorld(), !test.getVolume().fill());

	SelectionIntersection best;
	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i)
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
    assert(_numSelectedComponents > 0 == checkFaceInstancesForSelectedComponents(_faceInstances));

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
	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
		i->setSelected(mode, select);
	}
}

void BrushNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	// Component mode, invert the component selection
	switch (mode)
	{
	case selection::ComponentSelectionMode::Vertex:
		for (brush::VertexInstance& vertex : _vertexInstances)
		{
			vertex.invertSelected();
		}
		break;
	case selection::ComponentSelectionMode::Edge:
		for (EdgeInstance& edge : _edgeInstances)
		{
			edge.invertSelected();
		}
		break;
	case selection::ComponentSelectionMode::Face:
		for (FaceInstance& face : _faceInstances)
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

    switch (mode)
    {
    case selection::ComponentSelectionMode::Vertex:
        for (auto& vertex : _vertexInstances)
        {
            vertex.testSelect(selector, test);
        }
        break;

    case selection::ComponentSelectionMode::Edge:
        for (auto& edge : _edgeInstances)
        {
            edge.testSelect(selector, test);
        }
        break;

    case selection::ComponentSelectionMode::Face:
        if (test.getVolume().fill())
        {
            for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i)
            {
                i->testSelect(selector, test);
            }
        }
        else
        {
            for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i)
            {
                i->testSelect_centroid(selector, test);
            }
        }
    break;
    default: break;
    }
}

const AABB& BrushNode::getSelectedComponentsBounds() const {
	_aabb_component = AABB();

	for (FaceInstances::const_iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
		i->iterate_selected(_aabb_component);
	}

	return _aabb_component;
}

void BrushNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());

	PlanePointer brushPlanes[brush::c_brush_maxFaces];
	PlanesIterator j = brushPlanes;

	for (Brush::const_iterator i = _brush.begin(); i != _brush.end(); ++i) {
		*j++ = &(*i)->plane3();
	}

	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
		i->selectPlane(selector, Line(test.getNear(), test.getFar()), brushPlanes, j, selectedPlaneCallback);
	}
}

void BrushNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
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
	return _brush;
}

IBrush& BrushNode::getIBrush() {
	return _brush;
}

void BrushNode::translate(const Vector3& translation)
{
	_brush.translate(translation);
}

scene::INodePtr BrushNode::clone() const {
	return std::make_shared<BrushNode>(*this);
}

void BrushNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    _brush.connectUndoSystem(root.getUndoSystem());
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
    _brush.disconnectUndoSystem(root.getUndoSystem());
    _renderableVertices.clear();

	SelectableNode::onRemoveFromScene(root);
}

void BrushNode::clear() {
	_faceInstances.clear();
}

void BrushNode::reserve(std::size_t size) {
	_faceInstances.reserve(size);
}

void BrushNode::push_back(Face& face)
{
	_faceInstances.emplace_back(face, std::bind(&BrushNode::selectedChangedComponent, this, std::placeholders::_1));
    _untransformedOriginChanged = true;
}

void BrushNode::pop_back() {
	ASSERT_MESSAGE(!_faceInstances.empty(), "erasing invalid element");
	_faceInstances.pop_back();
    _untransformedOriginChanged = true;
}

void BrushNode::erase(std::size_t index) {
	ASSERT_MESSAGE(index < _faceInstances.size(), "erasing invalid element");
	_faceInstances.erase(_faceInstances.begin() + index);
}
void BrushNode::connectivityChanged() {
	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
		i->connectivityChanged();
	}
}

void BrushNode::edge_clear() {
	_edgeInstances.clear();
}
void BrushNode::edge_push_back(SelectableEdge& edge) {
	_edgeInstances.push_back(EdgeInstance(_faceInstances, edge));
}

void BrushNode::vertex_clear() {
	_vertexInstances.clear();
}
void BrushNode::vertex_push_back(SelectableVertex& vertex) {
	_vertexInstances.push_back(brush::VertexInstance(_faceInstances, vertex));
}

void BrushNode::DEBUG_verify() {
	ASSERT_MESSAGE(_faceInstances.size() == _brush.DEBUG_size(), "FATAL: mismatch");
}

void BrushNode::onFaceNeedsRenderableUpdate()
{
    _facesNeedRenderableUpdate = true;
}

void BrushNode::onPreRender(const VolumeTest& volume)
{
    _brush.evaluateBRep();

    assert(_renderEntity);

    // Run the face updates only if requested
    if (_facesNeedRenderableUpdate)
    {
        _facesNeedRenderableUpdate = false;

        const auto& wireShader = getRenderState() == RenderState::Active ?
            _renderEntity->getWireShader() : _inactiveWireShader;

        // Every face is asked to run the rendering preparations
        // to link/unlink their geometry to/from the active shader
        for (auto& faceInstance : _faceInstances)
        {
            auto& face = faceInstance.getFace();

            face.getWindingSurfaceSolid().update(face.getFaceShader().getGLShader(), *_renderEntity);
            face.getWindingSurfaceWireframe().update(wireShader, *_renderEntity);
        }
    }

    if (isSelected() && GlobalSelectionSystem().getSelectionMode() == selection::SelectionMode::Component ||
        _numSelectedComponents > 0) // could be a single selected face without the brush being selected
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
    bool isInMergeMode = collector.hasHighlightFlag(IRenderableCollector::Highlight::MergeAction);
    bool forceVisible = isForcedVisible() || isInMergeMode;
    bool wholeBrushSelected = isSelected() || Node_isSelected(getParent());

    // Don't touch the primitive highlight flag in merge mode
    if (!isInMergeMode)
    {
        collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, wholeBrushSelected);
    }

    // Submit the renderable geometry for each face
    for (auto& faceInstance : _faceInstances)
    {
        // Skip invisible faces before traversing further
        if (!forceVisible && !faceInstance.faceIsVisible()) continue;

        Face& face = faceInstance.getFace();
        if (face.intersectVolume(volume))
        {
            bool highlight = wholeBrushSelected || forceVisible || faceInstance.selectedComponents();

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
        collector.addHighlightRenderable(_clipPlane, Matrix4::getIdentity());
    }

    collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, false);
}

void BrushNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	if (renderSystem)
	{
        _pointShader = renderSystem->capture(BuiltInShaderType::BigPoint);
        _inactiveWireShader = renderSystem->capture(BuiltInShaderType::WireframeInactive);
        _renderableVertices.queueUpdate();
	}
	else
	{
        _inactiveWireShader.reset();
        _pointShader.reset();
        _renderableVertices.clear();
	}

	_brush.setRenderSystem(renderSystem);
	_clipPlane.setRenderSystem(renderSystem);
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

    for (const auto& faceInstance : _faceInstances)
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
            for (auto face : _brush)
            {
                face->translate(getTranslation());
            }
        }
        else
        {
            auto transform = calculateTransform();

            if (transform != Matrix4::getIdentity())
            {
                _brush.transform(transform);
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
	return _brush.getIntersection(ray, intersection);
}

void BrushNode::updateFaceVisibility()
{
	// Trigger an update, the brush might not have any faces calculated so far
	_brush.evaluateBRep();

	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i)
	{
		i->updateFaceVisibility();
	}
}

void BrushNode::transformComponents(const Matrix4& matrix) {
	for (FaceInstances::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
		i->transformComponents(matrix);
	}
}

void BrushNode::setClipPlane(const Plane3& plane)
{
    if (_renderEntity)
    {
        _clipPlane.setPlane(_brush, plane, *_renderEntity);
    }
}

void BrushNode::forEachFaceInstance(const std::function<void(FaceInstance&)>& functor)
{
	std::for_each(_faceInstances.begin(), _faceInstances.end(), functor);
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
    _brush.evaluateBRep();
}

void BrushNode::onPostRedo()
{
    _brush.evaluateBRep();
}

void BrushNode::_onTransformationChanged()
{
    _brush.transformChanged();

    _renderableVertices.queueUpdate();
	_renderableComponentsNeedUpdate = true;
}

void BrushNode::_applyTransformation()
{
	_brush.revertTransform();
	evaluateTransform();
	_brush.freezeTransform();

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

    _clipPlane.clear();
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
        _clipPlane.clear();
    }
}

void BrushNode::onRenderStateChanged()
{
    SelectableNode::onRenderStateChanged();

    _facesNeedRenderableUpdate = true;

    forEachFaceInstance([](FaceInstance& face)
    {
        face.getFace().onBrushRenderStateChanged();
    });
}

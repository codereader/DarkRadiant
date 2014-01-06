#include "BrushNode.h"

#include "ivolumetest.h"
#include "ifilter.h"
#include "iradiant.h"
#include "icounter.h"
#include "ientity.h"
#include "math/Frustum.h"
#include <boost/bind.hpp>

// Constructor
BrushNode::BrushNode() :
	scene::SelectableNode(),
	m_lightList(&GlobalRenderSystem().attachLitObject(*this)),
	m_brush(*this,
			Callback(boost::bind(&BrushNode::evaluateTransform, this)),
			Callback(boost::bind(&SelectableNode::boundsChanged, this))),
	_selectedPoints(GL_POINTS),
	_faceCentroidPointsCulled(GL_POINTS),
	m_viewChanged(false),
	_renderableComponentsNeedUpdate(true)
{
	m_brush.attach(*this); // BrushObserver

	SelectableNode::setTransformChangedCallback(boost::bind(&BrushNode::lightsChanged, this));
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
	LitObject(other),
	Transformable(other),
	m_lightList(&GlobalRenderSystem().attachLitObject(*this)),
	m_brush(*this, other.m_brush,
			Callback(boost::bind(&BrushNode::evaluateTransform, this)),
			Callback(boost::bind(&SelectableNode::boundsChanged, this))),
	_selectedPoints(GL_POINTS),
	_faceCentroidPointsCulled(GL_POINTS),
	m_viewChanged(false),
	_renderableComponentsNeedUpdate(true)
{
	m_brush.attach(*this); // BrushObserver
}

BrushNode::~BrushNode()
{
	GlobalRenderSystem().detachLitObject(*this);
	m_brush.detach(*this); // BrushObserver
}

scene::INode::Type BrushNode::getNodeType() const
{
	return Type::Primitive;
}

void BrushNode::lightsChanged()
{
	m_lightList->setDirty();
}

const AABB& BrushNode::localAABB() const {
	return m_brush.localAABB();
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

void BrushNode::invertSelected()
{
	// Override default behaviour of SelectableNode, we have components

	// Check if we are in component mode or not
	if (GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive)
	{
		// Non-component mode, invert the selection of the whole brush
		SelectableNode::invertSelected();
	} 
	else 
	{
		// Component mode, invert the component selection
		switch (GlobalSelectionSystem().ComponentMode()) {
			case SelectionSystem::eVertex:
				for (VertexInstances::iterator i = m_vertexInstances.begin(); i != m_vertexInstances.end(); ++i)
				{
					i->invertSelected();
				}
				break;
			case SelectionSystem::eEdge:
				for (EdgeInstances::iterator i = m_edgeInstances.begin(); i != m_edgeInstances.end(); ++i)
				{
					i->invertSelected();
				}
				break;
			case SelectionSystem::eFace:
				for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
				{
					i->invertSelected();
				}
				break;
			case SelectionSystem::eDefault:
				break;
		} // switch
	}
}

void BrushNode::testSelect(Selector& selector, SelectionTest& test) {
	test.BeginMesh(localToWorld());

	SelectionIntersection best;
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
	{
		if (i->faceIsVisible())
		{
			i->testSelect(test, best);
		}
	}

	if (best.valid()) {
		selector.addIntersection(best);
	}
}

bool BrushNode::isSelectedComponents() const {
	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if (i->selectedComponents()) {
			return true;
		}
	}
	return false;
}

void BrushNode::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->setSelected(mode, select);
	}
}

void BrushNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	test.BeginMesh(localToWorld());

	switch (mode) {
		case SelectionSystem::eVertex: {
				for (VertexInstances::iterator i = m_vertexInstances.begin(); i != m_vertexInstances.end(); ++i) {
					i->testSelect(selector, test);
				}
			}
		break;
		case SelectionSystem::eEdge: {
				for (EdgeInstances::iterator i = m_edgeInstances.begin(); i != m_edgeInstances.end(); ++i) {
					i->testSelect(selector, test);
				}
			}
			break;
		case SelectionSystem::eFace: {
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

	PlanePointer brushPlanes[c_brush_maxFaces];
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

void BrushNode::selectedChangedComponent(const Selectable& selectable)
{
	_renderableComponentsNeedUpdate = true;

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
	return scene::INodePtr(new BrushNode(*this));
}

void BrushNode::onInsertIntoScene()
{
	m_brush.instanceAttach(scene::findMapFile(getSelf()));
	GlobalCounters().getCounter(counterBrushes).increment();

	SelectableNode::onInsertIntoScene();
}

void BrushNode::onRemoveFromScene()
{
	// De-select this node
	setSelected(false);

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);
	setSelectedComponents(false, SelectionSystem::eEdge);
	setSelectedComponents(false, SelectionSystem::eFace);

	GlobalCounters().getCounter(counterBrushes).decrement();
	m_brush.instanceDetach(scene::findMapFile(getSelf()));

	SelectableNode::onRemoveFromScene();
}

void BrushNode::clear() {
	m_faceInstances.clear();
}

void BrushNode::reserve(std::size_t size) {
	m_faceInstances.reserve(size);
}

void BrushNode::push_back(Face& face) {
	m_faceInstances.push_back(FaceInstance(face, boost::bind(&BrushNode::selectedChangedComponent, this, _1)));
}

void BrushNode::pop_back() {
	ASSERT_MESSAGE(!m_faceInstances.empty(), "erasing invalid element");
	m_faceInstances.pop_back();
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

bool BrushNode::intersectsLight(const RendererLight& light) const {
	return light.intersectsAABB(worldAABB());
}

void BrushNode::insertLight(const RendererLight& light) {
	const Matrix4& l2w = localToWorld();
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->addLight(l2w, light);
	}
}

void BrushNode::clearLights() {
	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->m_lights.clear();
	}
}

void BrushNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const {
	m_brush.evaluateBRep();

	const Matrix4& l2w = localToWorld();

	collector.SetState(m_brush.m_state_point, RenderableCollector::eWireframeOnly);
	collector.SetState(m_brush.m_state_point, RenderableCollector::eFullMaterials);

	if (volume.fill() && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace) {
		evaluateViewDependent(volume, l2w);
		collector.addRenderable(_faceCentroidPointsCulled, l2w);
	}
	else {
		m_brush.renderComponents(GlobalSelectionSystem().ComponentMode(), collector, volume, l2w);
	}
}

void BrushNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const {
	m_brush.evaluateBRep();

	renderClipPlane(collector, volume);

	renderSolid(collector, volume, localToWorld());
}

void BrushNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const {
	m_brush.evaluateBRep();

	renderClipPlane(collector, volume);

	renderWireframe(collector, volume, localToWorld());
}

void BrushNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	if (renderSystem)
	{
		m_state_selpoint = renderSystem->capture("$SELPOINT");
	}
	else
	{
		m_state_selpoint.reset();
	}

	m_brush.setRenderSystem(renderSystem);
	m_clipPlane.setRenderSystem(renderSystem);
}

void BrushNode::renderClipPlane(RenderableCollector& collector, const VolumeTest& volume) const {
	if (GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip && isSelected()) {
		m_clipPlane.render(collector, volume, localToWorld());
	}
}

void BrushNode::viewChanged() const {
	m_viewChanged = true;
}

bool BrushNode::isHighlighted() const
{
	return isSelected();
}

void BrushNode::evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const
{
	if (!m_viewChanged) return;

	m_viewChanged = false;

	// Array of booleans to indicate which faces are visible
	static bool faces_visible[c_brush_maxFaces];

	// Will hold the indices of all visible faces (from the current viewpoint)
	static std::size_t visibleFaceIndices[c_brush_maxFaces];

	std::size_t numVisibleFaces(0);
	bool* j = faces_visible;

	// Iterator to an index of a visible face
	std::size_t* visibleFaceIter = visibleFaceIndices;
	std::size_t curFaceIndex = 0;

	for (FaceInstances::const_iterator i = m_faceInstances.begin();
		 i != m_faceInstances.end();
		 ++i, ++j, ++curFaceIndex)
	{
		// Check if face is filtered before adding to visibility matrix
		// greebo: Removed localToWorld transformation here, brushes don't have a non-identity l2w
		if (i->faceIsVisible() && i->intersectVolume(volume))
		{
			*j = true;

			// Store the index of this visible face in the array
			*visibleFaceIter++ = curFaceIndex;
			numVisibleFaces++;
		}
		else
		{
			*j = false;
		}
	}

	m_brush.update_wireframe(m_render_wireframe, faces_visible);
	m_brush.update_faces_wireframe(_faceCentroidPointsCulled, visibleFaceIndices, numVisibleFaces);
}

void BrushNode::renderSolid(RenderableCollector& collector,
                            const VolumeTest& volume,
                            const Matrix4& localToWorld) const
{
	m_lightList->calculateIntersectingLights();

	assert(_renderEntity); // brushes rendered without parent entity - no way!

    // Submit the lights and renderable geometry for each face
	for (FaceInstances::const_iterator i = m_faceInstances.begin();
         i != m_faceInstances.end();
         ++i)
    {
		// Skip invisible faces before traversing further
		if (!i->faceIsVisible()) continue;

        collector.setLights(i->m_lights);

		// greebo: BrushNodes have always an identity l2w, don't do any transforms
		i->submitRenderables(collector, volume, *_renderEntity);
    }

	renderSelectedPoints(collector, volume, localToWorld);
}

void BrushNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const {
	//renderCommon(collector, volume);

	evaluateViewDependent(volume, localToWorld);

	if (m_render_wireframe.m_size != 0) {
		collector.addRenderable(m_render_wireframe, localToWorld);
	}

	renderSelectedPoints(collector, volume, localToWorld);
}

void BrushNode::update_selected() const
{
	if (!_renderableComponentsNeedUpdate) return;

	_renderableComponentsNeedUpdate = false;

	_selectedPoints.clear();

	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if (i->getFace().contributes()) {
			i->iterate_selected(_selectedPoints);
		}
	}
}

void BrushNode::renderSelectedPoints(RenderableCollector& collector,
                                         const VolumeTest& volume,
                                         const Matrix4& localToWorld) const
{
	m_brush.evaluateBRep();

	update_selected();
	if (!_selectedPoints.empty())
    {
		collector.highlightPrimitives(false);
		collector.SetState(BrushNode::m_state_selpoint, RenderableCollector::eWireframeOnly);
		collector.SetState(BrushNode::m_state_selpoint, RenderableCollector::eFullMaterials);
		collector.addRenderable(_selectedPoints, localToWorld);
	}
}

void BrushNode::evaluateTransform() {
	Matrix4 matrix(calculateTransform());
	//rMessage() << "matrix: " << matrix << "\n";

	if (getType() == TRANSFORM_PRIMITIVE) {
		m_brush.transform(matrix);
	}
	else {
		transformComponents(matrix);
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

void BrushNode::setClipPlane(const Plane3& plane) {
	m_clipPlane.setPlane(m_brush, plane);
}

void BrushNode::forEachFaceInstance(const std::function<void(FaceInstance&)>& functor)
{
	std::for_each(m_faceInstances.begin(), m_faceInstances.end(), functor);
}

void BrushNode::_onTransformationChanged()
{
	m_brush.transformChanged();

	_renderableComponentsNeedUpdate = true;
}

void BrushNode::_applyTransformation()
{
	m_brush.revertTransform();
	evaluateTransform();
	m_brush.freezeTransform();
}

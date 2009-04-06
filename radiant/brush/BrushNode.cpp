#include "BrushNode.h"

#include "ifilter.h"
#include "iradiant.h"
#include "icounter.h"
#include "math/frustum.h"

// Constructor
BrushNode::BrushNode() :
	BrushTokenImporter(m_brush),
	BrushTokenExporter(m_brush),
	TransformModifier(Brush::TransformChangedCaller(m_brush), ApplyTransformCaller(*this)),
	m_brush(*this, EvaluateTransformCaller(*this), Node::BoundsChangedCaller(*this)),
	_selectable(SelectedChangedCaller(*this)),
	m_render_selected(GL_POINTS),
	m_render_faces_wireframe(m_faceCentroidPointsCulled, GL_POINTS),
	m_viewChanged(false)
{
	m_brush.attach(*this); // BrushObserver
	m_lightList = &GlobalRenderSystem().attach(*this);

	m_brush.m_lightsChanged = LightsChangedCaller(*this);

	Node::setTransformChangedCallback(LightsChangedCaller(*this));
}

// Copy Constructor
BrushNode::BrushNode(const BrushNode& other) :
	scene::Node(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	TransformNode(other),
	BrushDoom3(other),
	BrushTokenImporter(m_brush),
	BrushTokenExporter(m_brush),
	IBrushNode(other),
	Selectable(other),
	BrushObserver(other),
	SelectionTestable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	PlaneSelectable(other),
	LightCullable(other),
	Renderable(other),
	Cullable(other),
	Bounded(other),
	TransformModifier(Brush::TransformChangedCaller(m_brush), ApplyTransformCaller(*this)),
	m_brush(other.m_brush, *this, EvaluateTransformCaller(*this), Node::BoundsChangedCaller(*this)),
	_selectable(SelectedChangedCaller(*this)),
	m_render_selected(GL_POINTS),
	m_render_faces_wireframe(m_faceCentroidPointsCulled, GL_POINTS),
	m_viewChanged(false)
{
	m_brush.attach(*this); // BrushObserver
	m_lightList = &GlobalRenderSystem().attach(*this);
}

BrushNode::~BrushNode() {
	GlobalRenderSystem().detach(*this);
	m_brush.detach(*this); // BrushObserver
}

VolumeIntersectionValue BrushNode::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_brush.intersectVolume(test, localToWorld);
}

void BrushNode::lightsChanged() {
	m_lightList->lightsChanged();
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

// TransformNode implementation
const Matrix4& BrushNode::localToParent() const {
	return m_brush.localToParent();
}

bool BrushNode::isSelected() const {
	return _selectable.isSelected();
}

void BrushNode::setSelected(bool select) {
	_selectable.setSelected(select);
}

void BrushNode::invertSelected() {
	// Check if we are in component mode or not
	if (GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive) {
		// Non-component mode, invert the selection of the whole brush
		_selectable.invertSelected();
	} else {
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
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if (i->getFace().getShader().getGLShader()->getMaterial()->isVisible()) {
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

void BrushNode::onRemoveFromScene() {
	// De-select this node
	setSelected(false);

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);
	setSelectedComponents(false, SelectionSystem::eEdge);
	setSelectedComponents(false, SelectionSystem::eFace);
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

void BrushNode::selectedChanged(const Selectable& selectable) {
	GlobalSelectionSystem().onSelectedChanged(Node::getSelf(), selectable);

	// TODO? instance->selectedChanged();
}

void BrushNode::selectedChangedComponent(const Selectable& selectable) {
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

// IBrushNode implementation
Brush& BrushNode::getBrush() {
	return m_brush;
}

IBrush& BrushNode::getIBrush() {
	return m_brush;
}

void BrushNode::translateDoom3Brush(const Vector3& translation) {
	m_brush.translateDoom3Brush(translation);
}

scene::INodePtr BrushNode::clone() const {
	scene::INodePtr clone(new BrushNode(*this));
	clone->setSelf(clone);
	return clone;
}

void BrushNode::instantiate(const scene::Path& path) {
	m_brush.instanceAttach(path);
	GlobalRadiant().getCounter(counterBrushes).increment();

	Node::instantiate(path);
}

void BrushNode::uninstantiate(const scene::Path& path) {
	GlobalRadiant().getCounter(counterBrushes).decrement();
	m_brush.instanceDetach(path);

	Node::uninstantiate(path);
}

void BrushNode::constructStatic() {
	m_state_selpoint = GlobalRenderSystem().capture("$SELPOINT");
}

void BrushNode::destroyStatic() {
	m_state_selpoint = ShaderPtr(); 
}

void BrushNode::clear() {
	m_faceInstances.clear();
}

void BrushNode::reserve(std::size_t size) {
	m_faceInstances.reserve(size);
}

void BrushNode::push_back(Face& face) {
	m_faceInstances.push_back(FaceInstance(face, SelectedChangedComponentCaller(*this)));
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

bool BrushNode::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
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
		collector.addRenderable(m_render_faces_wireframe, l2w);
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

void BrushNode::renderClipPlane(RenderableCollector& collector, const VolumeTest& volume) const {
	if (GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip && isSelected()) {
		m_clipPlane.render(collector, volume, localToWorld());
	}
}

void BrushNode::viewChanged() const {
	m_viewChanged = true;
}

void BrushNode::evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const {
	if (m_viewChanged) {
		m_viewChanged = false;

		bool faces_visible[c_brush_maxFaces];
		{
			bool* j = faces_visible;
			for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i, ++j) {
				// Check if face is filtered before adding to visibility matrix
				if (i->getFace().getShader().getGLShader()->getMaterial()->isVisible())
					*j = i->intersectVolume(volume, localToWorld);
				else
					*j = false;
			}
		}

		m_brush.update_wireframe(m_render_wireframe, faces_visible);
		m_brush.update_faces_wireframe(m_faceCentroidPointsCulled, faces_visible);
	}
}

void BrushNode::renderSolid(RenderableCollector& collector,
                            const VolumeTest& volume,
                            const Matrix4& localToWorld) const 
{
	m_lightList->evaluateLights();

    // Submit the lights and renderable geometry for each face
	for (FaceInstances::const_iterator i = m_faceInstances.begin();
         i != m_faceInstances.end();
         ++i) 
    {
        collector.setLights(i->m_lights);
        i->submitRenderables(collector, volume, localToWorld);
    }

	renderComponentsSelected(collector, volume, localToWorld);
}

void BrushNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const {
	//renderCommon(collector, volume);

	evaluateViewDependent(volume, localToWorld);

	if (m_render_wireframe.m_size != 0) {
		collector.addRenderable(m_render_wireframe, localToWorld);
	}

	renderComponentsSelected(collector, volume, localToWorld);
}

void BrushNode::update_selected() const {
	m_render_selected.clear();

	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if (i->getFace().contributes()) {
			i->iterate_selected(m_render_selected);
		}
	}
}

void BrushNode::renderComponentsSelected(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const {
	m_brush.evaluateBRep();

	update_selected();
	if (!m_render_selected.empty()) {
		collector.Highlight(RenderableCollector::ePrimitive, false);
		collector.SetState(BrushNode::m_state_selpoint, RenderableCollector::eWireframeOnly);
		collector.SetState(BrushNode::m_state_selpoint, RenderableCollector::eFullMaterials);
		collector.addRenderable(m_render_selected, localToWorld);
	}
}

void BrushNode::evaluateTransform() {
	Matrix4 matrix(calculateTransform());
	//globalOutputStream() << "matrix: " << matrix << "\n";

	if (getType() == TRANSFORM_PRIMITIVE) {
		m_brush.transform(matrix);
	}
	else {
		transformComponents(matrix);
	}
}

void BrushNode::transformComponents(const Matrix4& matrix) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->transformComponents(matrix);
	}
}

void BrushNode::applyTransform() {
	m_brush.revertTransform();
	evaluateTransform();
	m_brush.freezeTransform();
}

void BrushNode::setClipPlane(const Plane3& plane) {
	m_clipPlane.setPlane(m_brush, plane);
}

const BrushInstanceVisitor& BrushNode::forEachFaceInstance(const BrushInstanceVisitor& visitor) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		visitor.visit(*i);
	}
	return visitor;
}

ShaderPtr BrushNode::m_state_selpoint;

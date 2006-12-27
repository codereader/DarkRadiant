#include "BrushInstance.h"

#include "ifilter.h"

BrushInstance::BrushInstance(const scene::Path& path, scene::Instance* parent, Brush& brush) :
		Instance(path, parent, this, StaticTypeCasts::instance().get()),
		m_brush(brush),
		m_selectable(SelectedChangedCaller(*this)),
		m_render_selected(GL_POINTS),
		m_render_faces_wireframe(m_faceCentroidPointsCulled, GL_POINTS),
		m_viewChanged(false),
		m_transform(Brush::TransformChangedCaller(m_brush), ApplyTransformCaller(*this)) {
	m_brush.instanceAttach(Instance::path());
	m_brush.attach(*this);
	m_counter->increment();

	m_lightList = &GlobalShaderCache().attach(*this);
	m_brush.m_lightsChanged = LightsChangedCaller(*this); ///\todo Make this work with instancing.

	Instance::setTransformChangedCallback(LightsChangedCaller(*this));
}

BrushInstance::~BrushInstance() {
	Instance::setTransformChangedCallback(Callback());

	m_brush.m_lightsChanged = Callback();
	GlobalShaderCache().detach(*this);

	m_counter->decrement();
	m_brush.detach(*this);
	m_brush.instanceDetach(Instance::path());
}

void BrushInstance::lightsChanged() {
	m_lightList->lightsChanged();
}

Brush& BrushInstance::getBrush() {
	return m_brush;
}
const Brush& BrushInstance::getBrush() const {
	return m_brush;
}

Bounded& BrushInstance::get(NullType<Bounded>) {
	return m_brush;
}
Cullable& BrushInstance::get(NullType<Cullable>) {
	return m_brush;
}
Transformable& BrushInstance::get(NullType<Transformable>) {
	return m_transform;
}

void BrushInstance::selectedChanged(const Selectable& selectable) {
	GlobalSelectionSystem().getObserver(SelectionSystem::ePrimitive)(selectable);
	GlobalSelectionSystem().onSelectedChanged(*this, selectable);

	Instance::selectedChanged();
}

void BrushInstance::selectedChangedComponent(const Selectable& selectable) {
	GlobalSelectionSystem().getObserver(SelectionSystem::eComponent)(selectable);
	GlobalSelectionSystem().onComponentSelection(*this, selectable);
}

const BrushInstanceVisitor& BrushInstance::forEachFaceInstance(const BrushInstanceVisitor& visitor) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		visitor.visit(*i);
	}
	return visitor;
}

void BrushInstance::constructStatic() {
	m_state_selpoint = GlobalShaderCache().capture("$SELPOINT");
}
void BrushInstance::destroyStatic() {
	GlobalShaderCache().release("$SELPOINT");
}

void BrushInstance::clear() {
	m_faceInstances.clear();
}
void BrushInstance::reserve(std::size_t size) {
	m_faceInstances.reserve(size);
}

void BrushInstance::push_back(Face& face) {
	m_faceInstances.push_back(FaceInstance(face, SelectedChangedComponentCaller(*this)));
}
void BrushInstance::pop_back() {
	ASSERT_MESSAGE(!m_faceInstances.empty(), "erasing invalid element");
	m_faceInstances.pop_back();
}
void BrushInstance::erase(std::size_t index) {
	ASSERT_MESSAGE(index < m_faceInstances.size(), "erasing invalid element");
	m_faceInstances.erase(m_faceInstances.begin() + index);
}
void BrushInstance::connectivityChanged() {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).connectivityChanged();
	}
}

void BrushInstance::edge_clear() {
	m_edgeInstances.clear();
}
void BrushInstance::edge_push_back(SelectableEdge& edge) {
	m_edgeInstances.push_back(EdgeInstance(m_faceInstances, edge));
}

void BrushInstance::vertex_clear() {
	m_vertexInstances.clear();
}
void BrushInstance::vertex_push_back(SelectableVertex& vertex) {
	m_vertexInstances.push_back(VertexInstance(m_faceInstances, vertex));
}

void BrushInstance::DEBUG_verify() const {
	ASSERT_MESSAGE(m_faceInstances.size() == m_brush.DEBUG_size(), "FATAL: mismatch");
}

bool BrushInstance::isSelected() const {
	return m_selectable.isSelected();
}

void BrushInstance::setSelected(bool select) {
	m_selectable.setSelected(select);
}

void BrushInstance::invertSelected() {
	// Check if we are in component mode or not
	if (GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive) {
		// Non-component mode, invert the selection of the whole brush
		m_selectable.invertSelected();
	} else {
		// Component mode, invert the component selection
		switch (GlobalSelectionSystem().ComponentMode()) {
			case SelectionSystem::eVertex:
				for (VertexInstances::iterator i = m_vertexInstances.begin(); i != m_vertexInstances.end(); ++i) {
					i->invertSelected();
				}
				break;
			case SelectionSystem::eEdge:
				for (EdgeInstances::iterator i = m_edgeInstances.begin(); i != m_edgeInstances.end(); ++i) {
					i->invertSelected();
				}
				break;
			case SelectionSystem::eFace:
				for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
					i->invertSelected();
				}
				break;
			case SelectionSystem::eDefault:
				break;
		} // switch
	}
}


void BrushInstance::update_selected() const {
	m_render_selected.clear();
	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if (i->getFace().contributes()) {
			i->iterate_selected(m_render_selected);
		}
	}
}

void BrushInstance::evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const {
	if (m_viewChanged) {
		m_viewChanged = false;

		bool faces_visible[c_brush_maxFaces];
		{
			bool* j = faces_visible;
			for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i, ++j) {
				// Check if face is filtered before adding to visibility matrix
				if (GlobalFilterSystem().isVisible("texture", i->getFace().GetShader()))
					*j = i->intersectVolume(volume, localToWorld);
				else
					*j = false;
			}
		}

		m_brush.update_wireframe(m_render_wireframe, faces_visible);
		m_brush.update_faces_wireframe(m_faceCentroidPointsCulled, faces_visible);
	}
}

void BrushInstance::renderComponentsSelected(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	m_brush.evaluateBRep();

	update_selected();
	if (!m_render_selected.empty()) {
		renderer.Highlight(Renderer::ePrimitive, false);
		renderer.SetState(m_state_selpoint, Renderer::eWireframeOnly);
		renderer.SetState(m_state_selpoint, Renderer::eFullMaterials);
		renderer.addRenderable(m_render_selected, localToWorld);
	}
}

void BrushInstance::renderComponents(Renderer& renderer, const VolumeTest& volume) const {
	m_brush.evaluateBRep();

	const Matrix4& localToWorld = Instance::localToWorld();

	renderer.SetState(m_brush.m_state_point, Renderer::eWireframeOnly);
	renderer.SetState(m_brush.m_state_point, Renderer::eFullMaterials);

	if (volume.fill() && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace) {
			evaluateViewDependent(volume, localToWorld);
			renderer.addRenderable(m_render_faces_wireframe, localToWorld);
		}
	else {
			m_brush.renderComponents(GlobalSelectionSystem().ComponentMode(), renderer, volume, localToWorld);
		}
}

void BrushInstance::renderClipPlane(Renderer& renderer, const VolumeTest& volume) const {
	if (GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip && isSelected()) {
			m_clipPlane.render(renderer, volume, localToWorld());
		}
}

void BrushInstance::renderCommon(Renderer& renderer, const VolumeTest& volume) const {
	bool componentMode = GlobalSelectionSystem().Mode() == SelectionSystem::eComponent;

	if (componentMode && isSelected()) {
			renderComponents(renderer, volume);
		}

	if (parentSelected()) {
			if (!componentMode) {
					renderer.Highlight(Renderer::eFace);
				}
			renderer.Highlight(Renderer::ePrimitive);
		}
}

void BrushInstance::renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	//renderCommon(renderer, volume);

	m_lightList->evaluateLights();

	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
			renderer.setLights((*i).m_lights);
			(*i).render(renderer, volume, localToWorld);
		}

	renderComponentsSelected(renderer, volume, localToWorld);
}

void BrushInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	//renderCommon(renderer, volume);

	evaluateViewDependent(volume, localToWorld);

	if (m_render_wireframe.m_size != 0) {
			renderer.addRenderable(m_render_wireframe, localToWorld);
		}

	renderComponentsSelected(renderer, volume, localToWorld);
}

void BrushInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	m_brush.evaluateBRep();

	renderClipPlane(renderer, volume);

	renderSolid(renderer, volume, localToWorld());
}

void BrushInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	m_brush.evaluateBRep();

	renderClipPlane(renderer, volume);

	renderWireframe(renderer, volume, localToWorld());
}

void BrushInstance::viewChanged() const {
	m_viewChanged = true;
}

void BrushInstance::testSelect(Selector& selector, SelectionTest& test) {
	test.BeginMesh(localToWorld());

	SelectionIntersection best;
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).testSelect(test, best);
	}
	if (best.valid()) {
		selector.addIntersection(best);
	}
}

bool BrushInstance::isSelectedComponents() const {
	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		if ((*i).selectedComponents()) {
			return true;
		}
	}
	return false;
}

void BrushInstance::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).setSelected(mode, select);
	}
}

void BrushInstance::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	test.BeginMesh(localToWorld());

	switch (mode) {
		case SelectionSystem::eVertex: {
				for (VertexInstances::iterator i = m_vertexInstances.begin(); i != m_vertexInstances.end(); ++i) {
					(*i).testSelect(selector, test);
				}
			}
			break;
		case SelectionSystem::eEdge: {
				for (EdgeInstances::iterator i = m_edgeInstances.begin(); i != m_edgeInstances.end(); ++i) {
					(*i).testSelect(selector, test);
				}
			}
			break;
		case SelectionSystem::eFace: {
				if (test.getVolume().fill()) {
					for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
						(*i).testSelect(selector, test);
					}
				}
				else {
					for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
						(*i).testSelect_centroid(selector, test);
					}
				}
			}
			break;
		default:
			break;
	}
}

void BrushInstance::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());

	PlanePointer brushPlanes[c_brush_maxFaces];
	PlanesIterator j = brushPlanes;

	for (Brush::const_iterator i = m_brush.begin(); i != m_brush.end(); ++i) {
		*j++ = &(*i)->plane3();
	}

	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).selectPlane(selector, Line(test.getNear(), test.getFar()), brushPlanes, j, selectedPlaneCallback);
	}
}

void BrushInstance::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).selectReversedPlane(selector, selectedPlanes);
	}
}


void BrushInstance::transformComponents(const Matrix4& matrix) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).transformComponents(matrix);
	}
}
const AABB& BrushInstance::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();

	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).iterate_selected(m_aabb_component);
	}

	return m_aabb_component;
}

void BrushInstance::snapComponents(float snap) {
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).snapComponents(snap);
	}
}

void BrushInstance::evaluateTransform() {
	Matrix4 matrix(m_transform.calculateTransform());
	//globalOutputStream() << "matrix: " << matrix << "\n";

	if (m_transform.getType() == TRANSFORM_PRIMITIVE) {
		m_brush.transform(matrix);
	}
	else {
		transformComponents(matrix);
	}
}
void BrushInstance::applyTransform() {
	m_brush.revertTransform();
	evaluateTransform();
	m_brush.freezeTransform();
}

void BrushInstance::setClipPlane(const Plane3& plane) {
	m_clipPlane.setPlane(m_brush, plane);
}

bool BrushInstance::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
}

void BrushInstance::insertLight(const RendererLight& light) {
	const Matrix4& localToWorld = Instance::localToWorld();
	for (FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		i->addLight(localToWorld, light);
	}
}

void BrushInstance::clearLights() {
	for (FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i) {
		(*i).m_lights.clear();
	}
}

// -------------------------------------------------------------------------------------------

Shader* BrushInstance::m_state_selpoint;
Counter* BrushInstance::m_counter = 0;
Shader* BrushClipPlane::m_state = 0;

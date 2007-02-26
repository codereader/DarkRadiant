#include "Doom3GroupInstance.h"

namespace entity {

class ControlPointAddBounds 
{
	AABB& m_bounds;
public:
	ControlPointAddBounds(AABB& bounds) : 
		m_bounds(bounds) 
	{}
	
	void operator()(const Vector3& point) const {
		m_bounds.includePoint(point);
	}
};

Doom3GroupInstance::Doom3GroupInstance(const scene::Path& path, 
									   scene::Instance* parent, 
									   Doom3Group& contained) :
	TargetableInstance(path, parent, this, StaticTypeCasts::instance().get(), contained.getEntity(), *this),
	TransformModifier(entity::Doom3Group::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
	m_contained(contained),
	m_curveNURBS(m_contained.m_curveNURBS.m_controlPointsTransformed, SelectionChangedComponentCaller(*this)),
	m_curveCatmullRom(m_contained.m_curveCatmullRom.m_controlPointsTransformed, SelectionChangedComponentCaller(*this)) {
	m_contained.instanceAttach(Instance::path());
	m_contained.m_curveNURBSChanged = m_contained.m_curveNURBS.connect(CurveEdit::CurveChangedCaller(m_curveNURBS));
	m_contained.m_curveCatmullRomChanged = m_contained.m_curveCatmullRom.connect(CurveEdit::CurveChangedCaller(m_curveCatmullRom));

	StaticRenderableConnectionLines::instance().attach(*this);
}

Doom3GroupInstance::~Doom3GroupInstance() {
	StaticRenderableConnectionLines::instance().detach(*this);

	m_contained.m_curveCatmullRom.disconnect(m_contained.m_curveCatmullRomChanged);
	m_contained.m_curveNURBS.disconnect(m_contained.m_curveNURBSChanged);
	m_contained.instanceDetach(Instance::path());
}

void Doom3GroupInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderSolid(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());

	m_curveNURBS.renderComponentsSelected(renderer, volume, localToWorld());
	m_curveCatmullRom.renderComponentsSelected(renderer, volume, localToWorld());
}

void Doom3GroupInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderWireframe(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());

	m_curveNURBS.renderComponentsSelected(renderer, volume, localToWorld());
	m_curveCatmullRom.renderComponentsSelected(renderer, volume, localToWorld());
}

void Doom3GroupInstance::renderComponents(Renderer& renderer, const VolumeTest& volume) const {
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		m_curveNURBS.renderComponents(renderer, volume, localToWorld());
		m_curveCatmullRom.renderComponents(renderer, volume, localToWorld());
	}
}

void Doom3GroupInstance::testSelect(Selector& selector, SelectionTest& test) {
	test.BeginMesh(localToWorld());
	SelectionIntersection best;

	// Pass the selection test to the Doom3Group class
	m_contained.testSelect(selector, test, best);

	// If the selectionIntersection is non-empty, add the selectable to the SelectionPool
	if (best.valid()) {
		Selector_add(selector, getSelectable(), best);
	}
}

bool Doom3GroupInstance::isSelectedComponents() const {
	return m_curveNURBS.isSelected() || m_curveCatmullRom.isSelected();
}

void Doom3GroupInstance::setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		m_curveNURBS.setSelected(selected);
		m_curveCatmullRom.setSelected(selected);
	}
}

void Doom3GroupInstance::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		test.BeginMesh(localToWorld());
		m_curveNURBS.testSelect(selector, test);
		m_curveCatmullRom.testSelect(selector, test);
	}
}

void Doom3GroupInstance::transformComponents(const Matrix4& matrix) {
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.transform(matrix);
	}
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.transform(matrix);
	}
}

const AABB& Doom3GroupInstance::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();
	m_curveNURBS.forEachSelected(ControlPointAddBounds(m_aabb_component));
	m_curveCatmullRom.forEachSelected(ControlPointAddBounds(m_aabb_component));
	return m_aabb_component;
}

void Doom3GroupInstance::snapComponents(float snap) {
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.snapto(snap);
		m_curveNURBS.write(curve_Nurbs, m_contained.getEntity());
	}
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.snapto(snap);
		m_curveCatmullRom.write(curve_CatmullRomSpline, m_contained.getEntity());
	}
}

void Doom3GroupInstance::evaluateTransform() {
	if (getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());
	}
	else {
		transformComponents(calculateTransform());
	}
}
void Doom3GroupInstance::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

void Doom3GroupInstance::selectionChangedComponent(const Selectable& selectable) {
	GlobalSelectionSystem().getObserver(SelectionSystem::eComponent)(selectable);
	GlobalSelectionSystem().onComponentSelection(*this, selectable);
}

Bounded& Doom3GroupInstance::get(NullType<Bounded>) {
	return m_contained;
}

} // namespace entity

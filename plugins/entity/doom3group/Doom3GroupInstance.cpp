#include "Doom3GroupInstance.h"

#include "../curve/CurveCatmullRom.h"
#include "../curve/CurveControlPointFunctors.h"

namespace entity {

Doom3GroupInstance::Doom3GroupInstance(const scene::Path& path, 
									   scene::Instance* parent, 
									   Doom3Group& contained) :
	TargetableInstance(path, parent, contained.getEntity(), *this),
	TransformModifier(Doom3Group::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
	m_contained(contained),
	m_curveNURBS(m_contained.m_curveNURBS,
				 SelectionChangedComponentCaller(*this)),
	m_curveCatmullRom(m_contained.m_curveCatmullRom, 
					  SelectionChangedComponentCaller(*this)),
	_originInstance(VertexInstance(m_contained.getOrigin(), SelectionChangedComponentCaller(*this)))
{
	m_contained.instanceAttach(Instance::path());
	m_contained.m_curveNURBSChanged = m_contained.m_curveNURBS.connect(
		CurveEditInstance::CurveChangedCaller(m_curveNURBS)
	);
	m_contained.m_curveCatmullRomChanged = m_contained.m_curveCatmullRom.connect(
		CurveEditInstance::CurveChangedCaller(m_curveCatmullRom)
	);

	StaticRenderableConnectionLines::instance().attach(*this);
}

Doom3GroupInstance::~Doom3GroupInstance() {
	StaticRenderableConnectionLines::instance().detach(*this);

	m_contained.m_curveCatmullRom.disconnect(m_contained.m_curveCatmullRomChanged);
	m_contained.m_curveNURBS.disconnect(m_contained.m_curveNURBSChanged);
	m_contained.instanceDetach(Instance::path());
}

bool Doom3GroupInstance::hasEmptyCurve() {
	return m_contained.m_curveNURBS.isEmpty() && 
		   m_contained.m_curveCatmullRom.isEmpty();
}

void Doom3GroupInstance::appendControlPoints(unsigned int numPoints) {
	m_contained.appendControlPoints(numPoints);
}

void Doom3GroupInstance::removeSelectedControlPoints() {
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.removeSelectedControlPoints();
		m_curveCatmullRom.write(curve_CatmullRomSpline, m_contained.getEntity());
	}
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.removeSelectedControlPoints();
		m_curveNURBS.write(curve_Nurbs, m_contained.getEntity());
	}
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
		
		// Register the renderable with OpenGL
		_originInstance.render(renderer, volume, Instance::localToWorld());
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
	return m_curveNURBS.isSelected() || m_curveCatmullRom.isSelected() || _originInstance.isSelected();
}

void Doom3GroupInstance::setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		m_curveNURBS.setSelected(selected);
		m_curveCatmullRom.setSelected(selected);
		_originInstance.setSelected(selected);
	}
}

void Doom3GroupInstance::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		test.BeginMesh(localToWorld());
		m_curveNURBS.testSelect(selector, test);
		m_curveCatmullRom.testSelect(selector, test);
		
		_originInstance.testSelect(selector, test);
	}
}

void Doom3GroupInstance::transformComponents(const Matrix4& matrix) {
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.transform(matrix);
	}
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.transform(matrix);
	}
	if (_originInstance.isSelected()) {
		m_contained.translateOrigin(getTranslation()); 
	}
}

const AABB& Doom3GroupInstance::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();
	
	ControlPointBoundsAdder boundsAdder(m_aabb_component);
	m_curveNURBS.forEachSelected(boundsAdder);
	m_curveCatmullRom.forEachSelected(boundsAdder);
	
	if (_originInstance.isSelected()) {
		m_aabb_component.includePoint(_originInstance.getVertex());
	}
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
	if (_originInstance.isSelected()) {
		m_contained.snapOrigin(snap);
	}
}

void Doom3GroupInstance::evaluateTransform() {
	if (getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(
			getTranslation(), 
			getRotation() != c_quaternion_identity // FALSE for identity rotations 
		);
		m_contained.rotate(getRotation());
		
		// Transform all curves also in primitive mode
		// pass FALSE to force the transformation of non-selected points
		Matrix4 transformation = calculateTransform();
		m_curveNURBS.transform(transformation, false);
		m_curveCatmullRom.transform(transformation, false);
	}
	else {
		// Transform the components only
		transformComponents(calculateTransform());
	}
	// Trigger a recalculation of the curve's controlpoints
	m_contained.m_curveNURBS.curveChanged();
	m_contained.m_curveCatmullRom.curveChanged();
}
void Doom3GroupInstance::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

void Doom3GroupInstance::selectionChangedComponent(const Selectable& selectable) {
	GlobalSelectionSystem().onComponentSelection(*this, selectable);
}

const AABB& Doom3GroupInstance::localAABB() const {
	return m_contained.localAABB();
}

} // namespace entity

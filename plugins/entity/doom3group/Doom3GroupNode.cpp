#include "Doom3GroupNode.h"

#include <boost/bind.hpp>
#include "../curve/CurveControlPointFunctors.h"

#include "Translatable.h"

namespace entity
{

Doom3GroupNode::Doom3GroupNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	_d3Group(
		*this, // Pass <this> as Doom3GroupNode&
		Callback(boost::bind(&scene::Node::boundsChanged, this))
	),
	_nurbsEditInstance(_d3Group.m_curveNURBS,
				 boost::bind(&Doom3GroupNode::selectionChangedComponent, this, _1)),
	_catmullRomEditInstance(_d3Group.m_curveCatmullRom,
					  boost::bind(&Doom3GroupNode::selectionChangedComponent, this, _1)),
	_originInstance(VertexInstance(_d3Group.getOrigin(), boost::bind(&Doom3GroupNode::selectionChangedComponent, this, _1)))
{}

Doom3GroupNode::Doom3GroupNode(const Doom3GroupNode& other) :
	EntityNode(other),
	scene::GroupNode(other),
	Snappable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	CurveNode(other),
	_d3Group(
		other._d3Group,
		*this, // Pass <this> as Doom3GroupNode&
		Callback(boost::bind(&scene::Node::boundsChanged, this))
	),
	_nurbsEditInstance(_d3Group.m_curveNURBS,
				 boost::bind(&Doom3GroupNode::selectionChangedComponent, this, _1)),
	_catmullRomEditInstance(_d3Group.m_curveCatmullRom,
					  boost::bind(&Doom3GroupNode::selectionChangedComponent, this, _1)),
	_originInstance(VertexInstance(_d3Group.getOrigin(), boost::bind(&Doom3GroupNode::selectionChangedComponent, this, _1)))
{
	// greebo: Don't call construct() here, this should be invoked by the
	// clone() method
}

Doom3GroupNodePtr Doom3GroupNode::Create(const IEntityClassPtr& eclass)
{
	Doom3GroupNodePtr instance(new Doom3GroupNode(eclass));
	instance->construct();

	return instance;
}

Doom3GroupNode::~Doom3GroupNode()
{ }

void Doom3GroupNode::construct()
{
    EntityNode::construct();

    _d3Group.construct();

    _d3Group.m_curveNURBS.signal_curveChanged().connect(
        sigc::mem_fun(_nurbsEditInstance, &CurveEditInstance::curveChanged)
    );
    _d3Group.m_curveCatmullRom.signal_curveChanged().connect(
        sigc::mem_fun(_catmullRomEditInstance, &CurveEditInstance::curveChanged)
    );
}

bool Doom3GroupNode::hasEmptyCurve() {
	return _d3Group.m_curveNURBS.isEmpty() &&
		   _d3Group.m_curveCatmullRom.isEmpty();
}

void Doom3GroupNode::appendControlPoints(unsigned int numPoints) {
	_d3Group.appendControlPoints(numPoints);
}

void Doom3GroupNode::removeSelectedControlPoints()
{
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.removeSelectedControlPoints();
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _entity);
	}
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.removeSelectedControlPoints();
		_nurbsEditInstance.write(curve_Nurbs, _entity);
	}
}

void Doom3GroupNode::insertControlPointsAtSelected() {
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.insertControlPointsAtSelected();
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _entity);
	}
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.insertControlPointsAtSelected();
		_nurbsEditInstance.write(curve_Nurbs, _entity);
	}
}

void Doom3GroupNode::convertCurveType() {
	_d3Group.convertCurveType();
}

const AABB& Doom3GroupNode::localAABB() const {
	return _d3Group.localAABB();
}

namespace
{

// Node visitor class to translate brushes
class BrushTranslator: public scene::NodeVisitor
{
    Vector3 m_origin;
public:
    BrushTranslator(const Vector3& origin) :
        m_origin(origin)
    {}

    bool pre(const scene::INodePtr& node)
    {
        Translatable* t = dynamic_cast<Translatable*>(node.get());
        if (t)
        {
            t->translate(m_origin);
        }
        return true;
    }
};

}

void Doom3GroupNode::addOriginToChildren()
{
	if (!_d3Group.isModel())
    {
		BrushTranslator translator(_d3Group.getOrigin());
		traverseChildren(translator);
	}
}

void Doom3GroupNode::removeOriginFromChildren()
{
	if (!_d3Group.isModel())
    {
		BrushTranslator translator(-_d3Group.getOrigin());
		traverseChildren(translator);
	}
}

void Doom3GroupNode::selectionChangedComponent(const Selectable& selectable) {
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

bool Doom3GroupNode::isSelectedComponents() const {
	return _nurbsEditInstance.isSelected() || _catmullRomEditInstance.isSelected() || (_d3Group.isModel() && _originInstance.isSelected());
}

void Doom3GroupNode::setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		_nurbsEditInstance.setSelected(selected);
		_catmullRomEditInstance.setSelected(selected);
		_originInstance.setSelected(selected);
	}
}

void Doom3GroupNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode)
{
	if (mode == SelectionSystem::eVertex)
	{
		test.BeginMesh(localToWorld());

		_originInstance.testSelect(selector, test);

		_nurbsEditInstance.testSelect(selector, test);
		_catmullRomEditInstance.testSelect(selector, test);
	}
}

const AABB& Doom3GroupNode::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();

	ControlPointBoundsAdder boundsAdder(m_aabb_component);
	_nurbsEditInstance.forEachSelected(boundsAdder);
	_catmullRomEditInstance.forEachSelected(boundsAdder);

	if (_originInstance.isSelected()) {
		m_aabb_component.includePoint(_originInstance.getVertex());
	}
	return m_aabb_component;
}

void Doom3GroupNode::snapComponents(float snap) {
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.snapto(snap);
		_nurbsEditInstance.write(curve_Nurbs, _entity);
	}
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.snapto(snap);
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _entity);
	}
	if (_originInstance.isSelected()) {
		_d3Group.snapOrigin(snap);
	}
}

scene::INodePtr Doom3GroupNode::clone() const
{
	Doom3GroupNodePtr clone(new Doom3GroupNode(*this));
	clone->construct();

	return clone;
}

void Doom3GroupNode::onInsertIntoScene()
{
	Node::instanceAttach(scene::findMapFile(getSelf()));

	EntityNode::onInsertIntoScene();
}

void Doom3GroupNode::onRemoveFromScene()
{
	// Call the base class first
	EntityNode::onRemoveFromScene();

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);

	Node::instanceDetach(scene::findMapFile(getSelf()));
}

// Snappable implementation
void Doom3GroupNode::snapto(float snap) {
	_d3Group.snapto(snap);
}

void Doom3GroupNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	test.BeginMesh(localToWorld());
	SelectionIntersection best;

	// Pass the selection test to the Doom3Group class
	_d3Group.testSelect(selector, test, best);

	// If the selectionIntersection is non-empty, add the selectable to the SelectionPool
	if (best.valid()) {
		Selector_add(selector, *this, best);
	}
}

void Doom3GroupNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	_d3Group.renderSolid(collector, volume, localToWorld(), isSelected());

	// Render curves always relative to the absolute map origin
	_nurbsEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
	_catmullRomEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
}

void Doom3GroupNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	_d3Group.renderWireframe(collector, volume, localToWorld(), isSelected());

	_nurbsEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
	_catmullRomEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
}

void Doom3GroupNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

	_d3Group.setRenderSystem(renderSystem);
	_nurbsEditInstance.setRenderSystem(renderSystem);
	_catmullRomEditInstance.setRenderSystem(renderSystem);

	_originInstance.setRenderSystem(renderSystem);
}

void Doom3GroupNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const
{
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex)
	{
		_nurbsEditInstance.renderComponents(collector, volume, Matrix4::getIdentity());

		_catmullRomEditInstance.renderComponents(collector, volume, Matrix4::getIdentity());

		// Register the renderable with OpenGL
		if (!_d3Group.isModel()) {
			_originInstance.render(collector, volume, localToWorld());
		}
	}
}

void Doom3GroupNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		const Quaternion& rotation = getRotation();
		const Vector3& scale = getScale();

		_d3Group.translate(
			getTranslation(),
			rotation != Quaternion::Identity(), // FALSE for identity rotations
			scale != c_scale_identity // FALSE for identity scales
		);
		_d3Group.rotate(rotation);
		_d3Group.scale(scale);

		// Transform curve control points in primitive mode
		Matrix4 transformation = calculateTransform();
		_nurbsEditInstance.transform(transformation, false);
		_catmullRomEditInstance.transform(transformation, false);
	}
	else {
		// Transform the components only
		transformComponents(calculateTransform());
	}
	// Trigger a recalculation of the curve's controlpoints
	_d3Group.m_curveNURBS.curveChanged();
	_d3Group.m_curveCatmullRom.curveChanged();
}

void Doom3GroupNode::transformComponents(const Matrix4& matrix)
{
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.transform(matrix);
	}

	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.transform(matrix);
	}

	if (_originInstance.isSelected()) {
		_d3Group.translateOrigin(getTranslation());
	}
}

void Doom3GroupNode::_onTransformationChanged()
{
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (!_d3Group.isModel())
	{
		scene::foreachTransformable(shared_from_this(), [] (ITransformable& child)
		{
			child.revertTransform();
		});

		evaluateTransform();

		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(_d3Group.getOrigin());
	}
	else
	{
		// It's a model
		_d3Group.revertTransform();
		evaluateTransform();
		_d3Group.updateTransform();
	}

	_d3Group.m_curveNURBS.curveChanged();
	_d3Group.m_curveCatmullRom.curveChanged();
}

void Doom3GroupNode::_applyTransformation()
{
	_d3Group.revertTransform();
	evaluateTransform();
	_d3Group.freezeTransform();

	if (!_d3Group.isModel())
	{
		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(_d3Group.getOrigin());
	}
}

void Doom3GroupNode::onModelKeyChanged(const std::string& value)
{
	// Override the default behaviour
	// Don't call EntityNode::onModelKeyChanged(value);

	// Pass the call to the contained model
	_d3Group.modelChanged(value);
}

} // namespace entity

#include "StaticGeometryNode.h"

#include <functional>
#include "../curve/CurveControlPointFunctors.h"

#include "Translatable.h"

namespace entity
{

StaticGeometryNode::StaticGeometryNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_originKey(std::bind(&StaticGeometryNode::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_rotationKey(std::bind(&StaticGeometryNode::rotationChanged, this)),
	_renderOrigin(m_origin),
	m_isModel(false),
	m_curveNURBS(*this, std::bind(&scene::Node::boundsChanged, this)),
	m_curveCatmullRom(*this, std::bind(&scene::Node::boundsChanged, this)),
	_nurbsEditInstance(m_curveNURBS,
				 std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_catmullRomEditInstance(m_curveCatmullRom,
					  std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_originInstance(getOrigin(), std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
    _nurbsVertices(m_curveNURBS, _nurbsEditInstance),
    _catmullRomVertices(m_curveCatmullRom, _catmullRomEditInstance),
    _renderableOriginVertex(_originInstance, localToWorld())
{}

StaticGeometryNode::StaticGeometryNode(const StaticGeometryNode& other) :
	EntityNode(other),
	scene::GroupNode(other),
	Snappable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	CurveNode(other),
	m_originKey(std::bind(&StaticGeometryNode::originChanged, this)),
	m_origin(other.m_origin),
	m_rotationKey(std::bind(&StaticGeometryNode::rotationChanged, this)),
	_renderOrigin(m_origin),
	m_isModel(other.m_isModel),
	m_curveNURBS(*this, std::bind(&scene::Node::boundsChanged, this)),
	m_curveCatmullRom(*this, std::bind(&scene::Node::boundsChanged, this)),
	_nurbsEditInstance(m_curveNURBS,
				 std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_catmullRomEditInstance(m_curveCatmullRom,
					  std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_originInstance(getOrigin(), std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
    _nurbsVertices(m_curveNURBS, _nurbsEditInstance),
    _catmullRomVertices(m_curveCatmullRom, _catmullRomEditInstance),
    _renderableOriginVertex(_originInstance, localToWorld())
{
	// greebo: Don't call construct() here, this should be invoked by the
	// clone() method
}

StaticGeometryNode::Ptr StaticGeometryNode::Create(const IEntityClassPtr& eclass)
{
	StaticGeometryNode::Ptr instance(new StaticGeometryNode(eclass));
	instance->construct();

	return instance;
}

StaticGeometryNode::~StaticGeometryNode()
{
	destroy();
}

void StaticGeometryNode::construct()
{
    EntityNode::construct();

	m_rotation.setIdentity();

    // Observe common spawnarg changes
    static_assert(std::is_base_of<sigc::trackable, RotationKey>::value);
    static_assert(std::is_base_of<sigc::trackable, StaticGeometryNode>::value);
    observeKey("origin", sigc::mem_fun(m_originKey, &OriginKey::onKeyValueChanged));
    observeKey("angle", sigc::mem_fun(m_rotationKey, &RotationKey::angleChanged));
    observeKey("rotation", sigc::mem_fun(m_rotationKey, &RotationKey::rotationChanged));
    observeKey("name", sigc::mem_fun(this, &StaticGeometryNode::nameChanged));

    // Observe curve-related spawnargs
    static_assert(std::is_base_of<sigc::trackable, CurveNURBS>::value);
    static_assert(std::is_base_of<sigc::trackable, CurveCatmullRom>::value);
    observeKey(curve_Nurbs, sigc::mem_fun(m_curveNURBS, &CurveNURBS::onKeyValueChanged));
    observeKey(curve_CatmullRomSpline,
               sigc::mem_fun(m_curveCatmullRom, &CurveCatmullRom::onKeyValueChanged));

    updateIsModel();

    m_curveNURBS.signal_curveChanged().connect(
        sigc::mem_fun(_nurbsEditInstance, &CurveEditInstance::curveChanged)
    );
    m_curveCatmullRom.signal_curveChanged().connect(
        sigc::mem_fun(_catmullRomEditInstance, &CurveEditInstance::curveChanged)
    );
}

void StaticGeometryNode::onVisibilityChanged(bool isVisibleNow)
{
    EntityNode::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        m_curveNURBS.updateRenderable();
        m_curveCatmullRom.updateRenderable();
        _nurbsVertices.queueUpdate();
        _catmullRomVertices.queueUpdate();
        _renderableOriginVertex.queueUpdate();
    }
    else
    {
        m_curveNURBS.clearRenderable();
        m_curveCatmullRom.clearRenderable();
        _nurbsVertices.clear();
        _catmullRomVertices.clear();
        _renderableOriginVertex.clear();
    }
}

void StaticGeometryNode::onSelectionStatusChange(bool changeGroupStatus)
{
    EntityNode::onSelectionStatusChange(changeGroupStatus);

    if (isSelected())
    {
        _renderOrigin.queueUpdate();
        _nurbsVertices.queueUpdate();
        _catmullRomVertices.queueUpdate();
        _renderableOriginVertex.queueUpdate();
    }
    else
    {
        _renderOrigin.clear();
        _nurbsVertices.clear();
        _catmullRomVertices.clear();
        _renderableOriginVertex.clear();
    }
}

bool StaticGeometryNode::hasEmptyCurve() {
	return m_curveNURBS.isEmpty() &&
		   m_curveCatmullRom.isEmpty();
}

void StaticGeometryNode::removeSelectedControlPoints()
{
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.removeSelectedControlPoints();
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _spawnArgs);
	}
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.removeSelectedControlPoints();
		_nurbsEditInstance.write(curve_Nurbs, _spawnArgs);
	}
}

void StaticGeometryNode::insertControlPointsAtSelected() {
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.insertControlPointsAtSelected();
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _spawnArgs);
	}
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.insertControlPointsAtSelected();
		_nurbsEditInstance.write(curve_Nurbs, _spawnArgs);
	}
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

void StaticGeometryNode::addOriginToChildren()
{
	if (!isModel())
    {
		BrushTranslator translator(getOrigin());
		traverseChildren(translator);
	}
}

void StaticGeometryNode::removeOriginFromChildren()
{
	if (!isModel())
    {
		BrushTranslator translator(-getOrigin());
		traverseChildren(translator);
	}
}

void StaticGeometryNode::selectionChangedComponent(const ISelectable& selectable)
{
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);

    _nurbsVertices.queueUpdate();
    _catmullRomVertices.queueUpdate();
    _renderableOriginVertex.queueUpdate();
}

bool StaticGeometryNode::isSelectedComponents() const {
	return _nurbsEditInstance.isSelected() || _catmullRomEditInstance.isSelected() || (isModel() && _originInstance.isSelected());
}

void StaticGeometryNode::setSelectedComponents(bool selected, selection::ComponentSelectionMode mode) {
	if (mode == selection::ComponentSelectionMode::Vertex) {
		_nurbsEditInstance.setSelected(selected);
		_catmullRomEditInstance.setSelected(selected);
		_originInstance.setSelected(selected);
	}
}

void StaticGeometryNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		_nurbsEditInstance.invertSelected();
		_catmullRomEditInstance.invertSelected();
		_originInstance.invertSelected();
	}
}

void StaticGeometryNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		test.BeginMesh(localToWorld());

		_originInstance.testSelect(selector, test);

		_nurbsEditInstance.testSelect(selector, test);
		_catmullRomEditInstance.testSelect(selector, test);
	}
}

const AABB& StaticGeometryNode::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();

	ControlPointBoundsAdder boundsAdder(m_aabb_component);
	_nurbsEditInstance.forEachSelected(boundsAdder);
	_catmullRomEditInstance.forEachSelected(boundsAdder);

	if (_originInstance.isSelected()) {
		m_aabb_component.includePoint(_originInstance.getVertex());
	}
	return m_aabb_component;
}

void StaticGeometryNode::snapComponents(float snap) {
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.snapto(snap);
		_nurbsEditInstance.write(curve_Nurbs, _spawnArgs);
	}
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.snapto(snap);
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _spawnArgs);
	}
	if (_originInstance.isSelected()) {
		snapOrigin(snap);
	}
}

scene::INodePtr StaticGeometryNode::clone() const
{
	StaticGeometryNode::Ptr clone(new StaticGeometryNode(*this));
	clone->construct();
    clone->constructClone(*this);

	return clone;
}

void StaticGeometryNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    EntityNode::onInsertIntoScene(root);

    // Clear curve renderables when hidden
    m_curveNURBS.updateRenderable();
    m_curveCatmullRom.updateRenderable();
}

void StaticGeometryNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	// Call the base class first
	EntityNode::onRemoveFromScene(root);

    // Clear curve renderables when hidden
    m_curveNURBS.clearRenderable();
    m_curveCatmullRom.clearRenderable();

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);
}

void StaticGeometryNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	test.BeginMesh(localToWorld());
	SelectionIntersection best;

	// Pass the selection test to the StaticGeometryNode class
	m_curveNURBS.testSelect(selector, test, best);
	m_curveCatmullRom.testSelect(selector, test, best);

	// If the selectionIntersection is non-empty, add the selectable to the SelectionPool
	if (best.isValid()) {
		selector.addWithIntersection(*this, best);
	}
}

void StaticGeometryNode::onPreRender(const VolumeTest& volume)
{
    EntityNode::onPreRender(volume);

    m_curveNURBS.onPreRender(getColourShader(), volume);
    m_curveCatmullRom.onPreRender(getColourShader(), volume);

    if (isSelected())
    {
        _renderOrigin.update(_pivotShader);

        if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
        {
            // Selected patches in component mode render the lattice connecting the control points
            _nurbsVertices.update(_pointShader);
            _catmullRomVertices.update(_pointShader);

            if (!isModel())
            {
                _renderableOriginVertex.update(_pointShader);
            }
            else
            {
                _renderableOriginVertex.clear();
            }
        }
        else
        {
            _nurbsVertices.clear();
            _catmullRomVertices.clear();
            _renderableOriginVertex.clear();

            // Queue an update the next time it's rendered
            _nurbsVertices.queueUpdate();
            _catmullRomVertices.queueUpdate();
            _renderableOriginVertex.queueUpdate();
        }
    }
}

void StaticGeometryNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    m_curveNURBS.renderHighlights(collector, volume);
    m_curveCatmullRom.renderHighlights(collector, volume);

    EntityNode::renderHighlights(collector, volume);
}

void StaticGeometryNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

    // Clear the geometry from any previous shader
    m_curveNURBS.clearRenderable();
    m_curveCatmullRom.clearRenderable();
    _nurbsVertices.clear();
    _catmullRomVertices.clear();
    _renderableOriginVertex.clear();

    if (renderSystem)
    {
        _pivotShader = renderSystem->capture(BuiltInShaderType::Pivot);
        _pointShader = renderSystem->capture(BuiltInShaderType::BigPoint);
    }
    else
    {
        _pivotShader.reset();
        _pointShader.reset();
    }
}

void StaticGeometryNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		const Quaternion& rotation = getRotation();
		const Vector3& scaleFactor = getScale();

        rotate(rotation);
		scale(scaleFactor);
		translate(getTranslation());

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
	m_curveNURBS.curveChanged();
	m_curveCatmullRom.curveChanged();
}

void StaticGeometryNode::transformComponents(const Matrix4& matrix)
{
	if (_nurbsEditInstance.isSelected())
    {
		_nurbsEditInstance.transform(matrix);
        _nurbsVertices.queueUpdate();
	}

	if (_catmullRomEditInstance.isSelected())
    {
		_catmullRomEditInstance.transform(matrix);
        _catmullRomVertices.queueUpdate();
	}

	if (_originInstance.isSelected())
    {
		translateOrigin(getTranslation());
        _renderableOriginVertex.queueUpdate();
	}
}

void StaticGeometryNode::_onTransformationChanged()
{
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (!isModel())
	{
        // Notify, any targeting nodes need to update their arrows pointing at us
        TargetableNode::onTransformationChanged();

		scene::forEachTransformable(*this, [] (ITransformable& child)
		{
			child.revertTransform();
		});

        revertTransformInternal();

		evaluateTransform();
	}
	else
	{
		// It's a model
		revertTransformInternal();
		evaluateTransform();
		updateTransform();
	}

	m_curveNURBS.curveChanged();
	m_curveCatmullRom.curveChanged();
    _nurbsVertices.queueUpdate();
    _catmullRomVertices.queueUpdate();
    _renderableOriginVertex.queueUpdate();
}

void StaticGeometryNode::_applyTransformation()
{
	revertTransformInternal();
	evaluateTransform();
	freezeTransformInternal();
}

void StaticGeometryNode::onModelKeyChanged(const std::string& value)
{
	// Override the default behaviour
	// Don't call EntityNode::onModelKeyChanged(value);

	// Pass the call to the contained model
	modelChanged(value);
}

inline void PointVertexArray_testSelect(VertexCb* first, std::size_t count,
	SelectionTest& test, SelectionIntersection& best)
{
	test.TestLineStrip(
	    VertexPointer(&first->vertex, sizeof(VertexCb)),
	    IndexPointer::index_type(count),
	    best
	);
}

Vector3& StaticGeometryNode::getOrigin() {
	return m_origin;
}

const Vector3& StaticGeometryNode::getUntransformedOrigin()
{
    return m_originKey.get();
}

const Vector3& StaticGeometryNode::getWorldPosition() const
{
    return m_origin;
}

const AABB& StaticGeometryNode::localAABB() const {
	m_curveBounds = m_curveNURBS.getBounds();
	m_curveBounds.includeAABB(m_curveCatmullRom.getBounds());

	if (m_curveBounds.isValid() || !m_isModel)
	{
		// Include the origin as well, it might be offset
		// Only do this, if the curve has valid bounds OR we have a non-Model,
		// otherwise we include the origin for models
		// and this AABB gets added to the children's
		// AABB in Instance::evaluateBounds(), which is wrong.
		m_curveBounds.includePoint(m_origin);
	}

	return m_curveBounds;
}

void StaticGeometryNode::snapOrigin(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
	_renderOrigin.queueUpdate();
}

void StaticGeometryNode::translateOrigin(const Vector3& translation)
{
	m_origin = m_originKey.get() + translation;
    _renderOrigin.queueUpdate();
}

void StaticGeometryNode::translate(const Vector3& translation)
{
	m_origin += translation;
    _renderOrigin.queueUpdate();
	translateChildren(translation);
}

void StaticGeometryNode::rotate(const Quaternion& rotation)
{
	if (!isModel())
	{
		// Rotate all child nodes too
		scene::forEachTransformable(*this, [&] (ITransformable& child)
		{
			child.setType(TRANSFORM_PRIMITIVE);
			child.setRotation(rotation);
		});

        m_origin = rotation.transformPoint(m_origin);
        _renderOrigin.queueUpdate();
	}
	else
	{
		m_rotation.rotate(rotation);
	}
}

void StaticGeometryNode::scale(const Vector3& scale)
{
	if (!isModel())
	{
		// Scale all child nodes too
		scene::forEachTransformable(*this, [&] (ITransformable& child)
		{
			child.setType(TRANSFORM_PRIMITIVE);
			child.setScale(scale);
		});

        m_origin *= scale;
        _renderOrigin.queueUpdate();
	}
}

void StaticGeometryNode::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
}

void StaticGeometryNode::revertTransformInternal()
{
	m_origin = m_originKey.get();

	if (isModel())
    {
		m_rotation = m_rotationKey.m_rotation;
	}

    _renderOrigin.queueUpdate();
	m_curveNURBS.revertTransform();
	m_curveCatmullRom.revertTransform();
}

void StaticGeometryNode::freezeTransformInternal()
{
	m_originKey.set(m_origin);
	m_originKey.write(_spawnArgs);

	if (!isModel())
	{
		scene::forEachTransformable(*this, [] (ITransformable& child)
		{
			child.freezeTransform();
		});
	}
	else
	{
		m_rotationKey.m_rotation = m_rotation;
		m_rotationKey.write(&_spawnArgs, isModel());
	}

	m_curveNURBS.freezeTransform();
	m_curveNURBS.saveToEntity(_spawnArgs);

	m_curveCatmullRom.freezeTransform();
	m_curveCatmullRom.saveToEntity(_spawnArgs);
}

void StaticGeometryNode::appendControlPoints(unsigned int numPoints) {
	if (!m_curveNURBS.isEmpty()) {
		m_curveNURBS.appendControlPoints(numPoints);
		m_curveNURBS.saveToEntity(_spawnArgs);
	}
	if (!m_curveCatmullRom.isEmpty()) {
		m_curveCatmullRom.appendControlPoints(numPoints);
		m_curveCatmullRom.saveToEntity(_spawnArgs);
	}
}

void StaticGeometryNode::convertCurveType() {
	if (!m_curveNURBS.isEmpty() && m_curveCatmullRom.isEmpty()) {
		std::string keyValue = _spawnArgs.getKeyValue(curve_Nurbs);
		_spawnArgs.setKeyValue(curve_Nurbs, "");
		_spawnArgs.setKeyValue(curve_CatmullRomSpline, keyValue);
	}
	else if (!m_curveCatmullRom.isEmpty() && m_curveNURBS.isEmpty()) {
		std::string keyValue = _spawnArgs.getKeyValue(curve_CatmullRomSpline);
		_spawnArgs.setKeyValue(curve_CatmullRomSpline, "");
		_spawnArgs.setKeyValue(curve_Nurbs, keyValue);
	}
}

void StaticGeometryNode::destroy()
{
	modelChanged("");
}

bool StaticGeometryNode::isModel() const {
	return m_isModel;
}

void StaticGeometryNode::setIsModel(bool newValue) {
	if (newValue && !m_isModel) {
		// The model key is not recognised as "name"
		getModelKey().modelChanged(m_modelKey);
	}
	else if (!newValue && m_isModel) {
		// Clear the model path
		getModelKey().modelChanged("");
	}
	m_isModel = newValue;
	updateTransform();
}

/** Determine if this StaticGeometryNode is a model (func_static) or a
 * brush-containing entity. If the "model" key is equal to the
 * "name" key, then this is a brush-based entity, otherwise it is
 * a model entity. The exception to this is the "worldspawn"
 * entity class, which is always a brush-based entity.
 */
void StaticGeometryNode::updateIsModel()
{
	if (m_modelKey != m_name && !_spawnArgs.isWorldspawn())
	{
		setIsModel(true);
	}
	else
	{
		setIsModel(false);
	}
}

void StaticGeometryNode::nameChanged(const std::string& value) {
	m_name = value;
	updateIsModel();
}

void StaticGeometryNode::modelChanged(const std::string& value)
{
	m_modelKey = value;
	updateIsModel();

	if (isModel())
	{
		getModelKey().modelChanged(value);
	}
	else
	{
		getModelKey().modelChanged("");
	}

    _renderOrigin.queueUpdate();
}

void StaticGeometryNode::updateTransform()
{
    if (isModel())
        setLocalToParent(Matrix4::getTranslation(m_origin) * m_rotation.getMatrix4());
    else
        setLocalToParent(Matrix4::getIdentity());

    // Notify the Node about this transformation change	to update the local2World matrix
    transformChanged();
}

void StaticGeometryNode::translateChildren(const Vector3& childTranslation)
{
	if (inScene())
	{
		// Translate all child nodes too
		scene::forEachTransformable(*this, [&] (ITransformable& child)
		{
			child.setType(TRANSFORM_PRIMITIVE);
			child.setTranslation(childTranslation);
		});
	}
}

void StaticGeometryNode::originChanged()
{
	m_origin = m_originKey.get();
	updateTransform();

    _renderableOriginVertex.queueUpdate();
    _renderOrigin.queueUpdate();
}

void StaticGeometryNode::rotationChanged() {
	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

} // namespace

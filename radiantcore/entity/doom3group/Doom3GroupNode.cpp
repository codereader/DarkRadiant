#include "Doom3GroupNode.h"

#include <functional>
#include "../curve/CurveControlPointFunctors.h"

#include "Translatable.h"

namespace entity
{

Doom3GroupNode::Doom3GroupNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_originKey(std::bind(&Doom3GroupNode::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_nameOrigin(0,0,0),
	m_rotationKey(std::bind(&Doom3GroupNode::rotationChanged, this)),
	m_renderOrigin(m_nameOrigin),
	m_isModel(false),
	m_curveNURBS(std::bind(&scene::Node::boundsChanged, this)),
	m_curveCatmullRom(std::bind(&scene::Node::boundsChanged, this)),
	_nurbsEditInstance(m_curveNURBS,
				 std::bind(&Doom3GroupNode::selectionChangedComponent, this, std::placeholders::_1)),
	_catmullRomEditInstance(m_curveCatmullRom,
					  std::bind(&Doom3GroupNode::selectionChangedComponent, this, std::placeholders::_1)),
	_originInstance(VertexInstance(getOrigin(), std::bind(&Doom3GroupNode::selectionChangedComponent, this, std::placeholders::_1)))
{}

Doom3GroupNode::Doom3GroupNode(const Doom3GroupNode& other) :
	EntityNode(other),
	scene::GroupNode(other),
	Snappable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	CurveNode(other),
	m_originKey(std::bind(&Doom3GroupNode::originChanged, this)),
	m_origin(other.m_origin),
	m_nameOrigin(other.m_nameOrigin),
	m_rotationKey(std::bind(&Doom3GroupNode::rotationChanged, this)),
	m_renderOrigin(m_nameOrigin),
	m_isModel(other.m_isModel),
	m_curveNURBS(std::bind(&scene::Node::boundsChanged, this)),
	m_curveCatmullRom(std::bind(&scene::Node::boundsChanged, this)),
	_nurbsEditInstance(m_curveNURBS,
				 std::bind(&Doom3GroupNode::selectionChangedComponent, this, std::placeholders::_1)),
	_catmullRomEditInstance(m_curveCatmullRom,
					  std::bind(&Doom3GroupNode::selectionChangedComponent, this, std::placeholders::_1)),
	_originInstance(VertexInstance(getOrigin(), std::bind(&Doom3GroupNode::selectionChangedComponent, this, std::placeholders::_1)))
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
{
	destroy();
}

void Doom3GroupNode::construct()
{
    EntityNode::construct();

	m_rotation.setIdentity();

    // Observe relevant spawnarg changes
	addKeyObserver("origin", m_originKey);
    observeKey("angle", [=](const std::string& val) { m_rotationKey.angleChanged(val); });
    observeKey("rotation", [=](const std::string& val) { m_rotationKey.rotationChanged(val); });
    observeKey("name", [=](const std::string& val) { nameChanged(val); });
	addKeyObserver(curve_Nurbs, m_curveNURBS);
	addKeyObserver(curve_CatmullRomSpline, m_curveCatmullRom);

	updateIsModel();

    m_curveNURBS.signal_curveChanged().connect(
        sigc::mem_fun(_nurbsEditInstance, &CurveEditInstance::curveChanged)
    );
    m_curveCatmullRom.signal_curveChanged().connect(
        sigc::mem_fun(_catmullRomEditInstance, &CurveEditInstance::curveChanged)
    );
}

void Doom3GroupNode::onVisibilityChanged(bool isVisibleNow)
{
    EntityNode::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        m_curveNURBS.updateRenderable();
        m_curveCatmullRom.updateRenderable();
    }
    else
    {
        m_curveNURBS.clearRenderable();
        m_curveCatmullRom.clearRenderable();
    }
}

bool Doom3GroupNode::hasEmptyCurve() {
	return m_curveNURBS.isEmpty() &&
		   m_curveCatmullRom.isEmpty();
}

void Doom3GroupNode::removeSelectedControlPoints()
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

void Doom3GroupNode::insertControlPointsAtSelected() {
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

void Doom3GroupNode::addOriginToChildren()
{
	if (!isModel())
    {
		BrushTranslator translator(getOrigin());
		traverseChildren(translator);
	}
}

void Doom3GroupNode::removeOriginFromChildren()
{
	if (!isModel())
    {
		BrushTranslator translator(-getOrigin());
		traverseChildren(translator);
	}
}

void Doom3GroupNode::selectionChangedComponent(const ISelectable& selectable) {
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

bool Doom3GroupNode::isSelectedComponents() const {
	return _nurbsEditInstance.isSelected() || _catmullRomEditInstance.isSelected() || (isModel() && _originInstance.isSelected());
}

void Doom3GroupNode::setSelectedComponents(bool selected, selection::ComponentSelectionMode mode) {
	if (mode == selection::ComponentSelectionMode::Vertex) {
		_nurbsEditInstance.setSelected(selected);
		_catmullRomEditInstance.setSelected(selected);
		_originInstance.setSelected(selected);
	}
}

void Doom3GroupNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		_nurbsEditInstance.invertSelected();
		_catmullRomEditInstance.invertSelected();
		_originInstance.invertSelected();
	}
}

void Doom3GroupNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
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

scene::INodePtr Doom3GroupNode::clone() const
{
	Doom3GroupNodePtr clone(new Doom3GroupNode(*this));
	clone->construct();
    clone->constructClone(*this);

	return clone;
}

void Doom3GroupNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    EntityNode::onInsertIntoScene(root);

    // Clear curve renderables when hidden
    m_curveNURBS.updateRenderable();
    m_curveCatmullRom.updateRenderable();
}

void Doom3GroupNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	// Call the base class first
	EntityNode::onRemoveFromScene(root);

    // Clear curve renderables when hidden
    m_curveNURBS.clearRenderable();
    m_curveCatmullRom.clearRenderable();

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);
}

void Doom3GroupNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	test.BeginMesh(localToWorld());
	SelectionIntersection best;

	// Pass the selection test to the Doom3GroupNode class
	m_curveNURBS.testSelect(selector, test, best);
	m_curveCatmullRom.testSelect(selector, test, best);

	// If the selectionIntersection is non-empty, add the selectable to the SelectionPool
	if (best.isValid()) {
		Selector_add(selector, *this, best);
	}
}

void Doom3GroupNode::onPreRender(const VolumeTest& volume)
{
    EntityNode::onPreRender(volume);

    m_curveNURBS.onPreRender(getColourShader(), volume);
    m_curveCatmullRom.onPreRender(getColourShader(), volume);
}

void Doom3GroupNode::renderCommon(IRenderableCollector& collector, const VolumeTest& volume) const
{
	if (isSelected())
    {
		m_renderOrigin.render(collector, volume, localToWorld());
	}

    // Render curves always relative to the absolute map origin
    static Matrix4 identity = Matrix4::getIdentity();
    _nurbsEditInstance.renderComponentsSelected(collector, volume, identity);
    _catmullRomEditInstance.renderComponentsSelected(collector, volume, identity);
}

void Doom3GroupNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);
    
    renderCommon(collector, volume);
}

void Doom3GroupNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);
 
    renderCommon(collector, volume);
}

void Doom3GroupNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    m_curveNURBS.renderHighlights(collector, volume);
    m_curveCatmullRom.renderHighlights(collector, volume);

    EntityNode::renderHighlights(collector, volume);
}

void Doom3GroupNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

    // Clear the geometry from any previous shader
    m_curveNURBS.clearRenderable();
    m_curveCatmullRom.clearRenderable();

	m_renderOrigin.setRenderSystem(renderSystem);
	_nurbsEditInstance.setRenderSystem(renderSystem);
	_catmullRomEditInstance.setRenderSystem(renderSystem);

	_originInstance.setRenderSystem(renderSystem);
}

void Doom3GroupNode::renderComponents(IRenderableCollector& collector, const VolumeTest& volume) const
{
	if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
	{
		_nurbsEditInstance.renderComponents(collector, volume, Matrix4::getIdentity());

		_catmullRomEditInstance.renderComponents(collector, volume, Matrix4::getIdentity());

		// Register the renderable with OpenGL
		if (!isModel()) {
			_originInstance.render(collector, volume, localToWorld());
		}
	}
}

void Doom3GroupNode::evaluateTransform()
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

void Doom3GroupNode::transformComponents(const Matrix4& matrix)
{
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.transform(matrix);
	}

	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.transform(matrix);
	}

	if (_originInstance.isSelected()) {
		translateOrigin(getTranslation());
	}
}

void Doom3GroupNode::_onTransformationChanged()
{
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (!isModel())
	{
		scene::forEachTransformable(*this, [] (ITransformable& child)
		{
			child.revertTransform();
		});

        revertTransform();

		evaluateTransform();

		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(getOrigin());
	}
	else
	{
		// It's a model
		revertTransform();
		evaluateTransform();
		updateTransform();
	}

	m_curveNURBS.curveChanged();
	m_curveCatmullRom.curveChanged();
}

void Doom3GroupNode::_applyTransformation()
{
	revertTransform();
	evaluateTransform();
	freezeTransform();

	if (!isModel())
	{
		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(getOrigin());
	}
}

void Doom3GroupNode::onModelKeyChanged(const std::string& value)
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

Vector3& Doom3GroupNode::getOrigin() {
	return m_origin;
}

const Vector3& Doom3GroupNode::getUntransformedOrigin()
{
    return m_originKey.get();
}

const AABB& Doom3GroupNode::localAABB() const {
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

void Doom3GroupNode::snapOrigin(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
	m_renderOrigin.updatePivot();
}

void Doom3GroupNode::translateOrigin(const Vector3& translation)
{
	m_origin = m_originKey.get() + translation;

	// Only non-models should have their rendered origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
}

void Doom3GroupNode::translate(const Vector3& translation)
{
	m_origin += translation;

	// Only non-models should have their rendered origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
	translateChildren(translation);
}

void Doom3GroupNode::rotate(const Quaternion& rotation)
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
        m_nameOrigin = m_origin;
        m_renderOrigin.updatePivot();
	}
	else
	{
		m_rotation.rotate(rotation);
	}
}

void Doom3GroupNode::scale(const Vector3& scale)
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
        m_nameOrigin = m_origin;
        m_renderOrigin.updatePivot();
	}
}

void Doom3GroupNode::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
}

void Doom3GroupNode::revertTransform()
{
	m_origin = m_originKey.get();

	// Only non-models should have their origin different than <0,0,0>
	if (!isModel()) {
		m_nameOrigin = m_origin;
	}
	else {
		m_rotation = m_rotationKey.m_rotation;
	}

	m_renderOrigin.updatePivot();
	m_curveNURBS.revertTransform();
	m_curveCatmullRom.revertTransform();
}

void Doom3GroupNode::freezeTransform()
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

void Doom3GroupNode::appendControlPoints(unsigned int numPoints) {
	if (!m_curveNURBS.isEmpty()) {
		m_curveNURBS.appendControlPoints(numPoints);
		m_curveNURBS.saveToEntity(_spawnArgs);
	}
	if (!m_curveCatmullRom.isEmpty()) {
		m_curveCatmullRom.appendControlPoints(numPoints);
		m_curveCatmullRom.saveToEntity(_spawnArgs);
	}
}

void Doom3GroupNode::convertCurveType() {
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

void Doom3GroupNode::destroy()
{
	modelChanged("");

	removeKeyObserver("origin", m_originKey);
	removeKeyObserver(curve_Nurbs, m_curveNURBS);
	removeKeyObserver(curve_CatmullRomSpline, m_curveCatmullRom);
}

bool Doom3GroupNode::isModel() const {
	return m_isModel;
}

void Doom3GroupNode::setIsModel(bool newValue) {
	if (newValue && !m_isModel) {
		// The model key is not recognised as "name"
		getModelKey().modelChanged(m_modelKey);
	}
	else if (!newValue && m_isModel) {
		// Clear the model path
		getModelKey().modelChanged("");
		m_nameOrigin = m_origin;
	}
	m_isModel = newValue;
	updateTransform();
}

/** Determine if this Doom3GroupNode is a model (func_static) or a
 * brush-containing entity. If the "model" key is equal to the
 * "name" key, then this is a brush-based entity, otherwise it is
 * a model entity. The exception to this is the "worldspawn"
 * entity class, which is always a brush-based entity.
 */
void Doom3GroupNode::updateIsModel()
{
	if (m_modelKey != m_name && !_spawnArgs.isWorldspawn())
	{
		setIsModel(true);

		// Set the renderable name back to 0,0,0
		_renderableName.setOrigin(Vector3(0,0,0));
	}
	else
	{
		setIsModel(false);

		// Update the renderable name
		_renderableName.setOrigin(getOrigin());
	}
}

void Doom3GroupNode::nameChanged(const std::string& value) {
	m_name = value;
	updateIsModel();
	m_renderOrigin.updatePivot();
}

void Doom3GroupNode::modelChanged(const std::string& value)
{
	m_modelKey = value;
	updateIsModel();

	if (isModel())
	{
		getModelKey().modelChanged(value);
		m_nameOrigin = Vector3(0,0,0);
	}
	else
	{
		getModelKey().modelChanged("");
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
}

void Doom3GroupNode::updateTransform()
{
	localToParent() = Matrix4::getIdentity();

	if (isModel())
	{
		localToParent().translateBy(m_origin);
		localToParent().multiplyBy(m_rotation.getMatrix4());
	}

	// Notify the Node about this transformation change	to update the local2World matrix
	transformChanged();
}

void Doom3GroupNode::translateChildren(const Vector3& childTranslation)
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

void Doom3GroupNode::originChanged()
{
	m_origin = m_originKey.get();
	updateTransform();
	// Only non-models should have their origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
		// Update the renderable name
		_renderableName.setOrigin(getOrigin());
	}
	m_renderOrigin.updatePivot();
}

void Doom3GroupNode::rotationChanged() {
	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

} // namespace entity

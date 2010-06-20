#include "Doom3Group.h"

#include "iregistry.h"
#include "iselectable.h"
#include "render.h"
#include "transformlib.h"

#include "../EntitySettings.h"
#include "Doom3GroupNode.h"
#include <boost/bind.hpp>

namespace entity {

	namespace {
		const char* RKEY_FREE_MODEL_ROTATION = "user/ui/freeModelRotation";
	}

inline void PointVertexArray_testSelect(PointVertex* first, std::size_t count, 
	SelectionTest& test, SelectionIntersection& best) 
{
	test.TestLineStrip(
	    VertexPointer(&first->vertex, sizeof(PointVertex)),
	    IndexPointer::index_type(count),
	    best
	);
}

Doom3Group::Doom3Group(
		Doom3GroupNode& owner,
		const Callback& boundsChanged) :
	_owner(owner),
	_entity(_owner._entity),
	m_model(owner),
	m_originKey(boost::bind(&Doom3Group::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_nameOrigin(0,0,0),
	m_rotationKey(boost::bind(&Doom3Group::rotationChanged, this)),
	m_renderOrigin(m_nameOrigin),
	m_curveNURBS(boundsChanged),
	m_curveCatmullRom(boundsChanged)
{
	// construct() is called by the Doom3GroupNode
}

Doom3Group::Doom3Group(const Doom3Group& other, 
		Doom3GroupNode& owner,
		const Callback& boundsChanged) :
	_owner(owner),
	_entity(_owner._entity),
	m_model(owner),
	m_originKey(boost::bind(&Doom3Group::originChanged, this)),
	m_origin(other.m_origin),
	m_nameOrigin(other.m_nameOrigin),
	m_rotationKey(boost::bind(&Doom3Group::rotationChanged, this)),
	m_renderOrigin(m_nameOrigin),
	m_curveNURBS(boundsChanged),
	m_curveCatmullRom(boundsChanged)
{
	// construct() is called by the Doom3GroupNode
}

Doom3Group::~Doom3Group() {
	destroy();
}

Vector3& Doom3Group::getOrigin() {
	return m_origin;
}

const AABB& Doom3Group::localAABB() const {
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

void Doom3Group::renderSolid(RenderableCollector& collector, const VolumeTest& volume, 
	const Matrix4& localToWorld, bool selected) const 
{
	if (selected) {
		m_renderOrigin.render(collector, volume, localToWorld);
	}

	collector.SetState(_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
	collector.SetState(_entity.getEntityClass()->getWireShader(), RenderableCollector::eFullMaterials);

	if (!m_curveNURBS.isEmpty())
	{
		// Always render curves relative to map origin
		m_curveNURBS.renderSolid(collector, volume, Matrix4::getIdentity());
	}
	
	if (!m_curveCatmullRom.isEmpty())
	{
		// Always render curves relative to map origin
		m_curveCatmullRom.renderSolid(collector, volume, Matrix4::getIdentity());
	}
}

void Doom3Group::renderWireframe(RenderableCollector& collector, const VolumeTest& volume, 
	const Matrix4& localToWorld, bool selected) const 
{
	renderSolid(collector, volume, localToWorld, selected);
}

void Doom3Group::testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best)
{
	// Pass the call down to the model node, if applicable
	SelectionTestablePtr selectionTestable = Node_getSelectionTestable(m_model.getNode());

    if (selectionTestable)
	{
		selectionTestable->testSelect(selector, test);
    }

	m_curveNURBS.testSelect(selector, test, best);
	m_curveCatmullRom.testSelect(selector, test, best);
}

void Doom3Group::snapOrigin(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&_entity);
	m_renderOrigin.updatePivot();
}

void Doom3Group::translateOrigin(const Vector3& translation)
{
	m_origin = m_originKey.m_origin + translation;

	// Only non-models should have their rendered origin different than <0,0,0>
	if (!isModel()) {
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
}

void Doom3Group::translate(const Vector3& translation, bool rotation) {
	
	bool freeModelRotation = GlobalRegistry().get(RKEY_FREE_MODEL_ROTATION) == "1";
	
	// greebo: If the translation does not originate from 
	// a pivoted rotation, translate the origin as well (this is a bit hacky)
	// This also applies for models, which should always have the 
	// rotation-translation applied (except for freeModelRotation set to TRUE)
	if (!rotation || (isModel() && !freeModelRotation))
	{
		m_origin = m_originKey.m_origin + translation;
	}
	
	// Only non-models should have their rendered origin different than <0,0,0>
	if (!isModel()) {
		m_nameOrigin = m_origin;
	}
	m_renderOrigin.updatePivot();
	translateChildren(translation);
}

void Doom3Group::rotate(const Quaternion& rotation) {
	if (!isModel()) {
		ChildRotator rotator(rotation);
		_owner.traverse(rotator);
	}
	else {
		m_rotation.rotate(rotation);
	}
}

void Doom3Group::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&_entity);
}

void Doom3Group::revertTransform() {
	m_origin = m_originKey.m_origin;
	
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

void Doom3Group::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&_entity);
	
	if (!isModel()) {
		ChildTransformFreezer freezer;
		_owner.traverse(freezer);
	}
	else {
		m_rotationKey.m_rotation = m_rotation;
		m_rotationKey.write(&_entity, isModel());
	}
	m_curveNURBS.freezeTransform();
	m_curveNURBS.saveToEntity(_entity);
	
	m_curveCatmullRom.freezeTransform();
	m_curveCatmullRom.saveToEntity(_entity);
}

void Doom3Group::appendControlPoints(unsigned int numPoints) {
	if (!m_curveNURBS.isEmpty()) {
		m_curveNURBS.appendControlPoints(numPoints);
		m_curveNURBS.saveToEntity(_entity);
	}
	if (!m_curveCatmullRom.isEmpty()) {
		m_curveCatmullRom.appendControlPoints(numPoints);
		m_curveCatmullRom.saveToEntity(_entity);
	}
}

void Doom3Group::convertCurveType() {
	if (!m_curveNURBS.isEmpty() && m_curveCatmullRom.isEmpty()) {
		std::string keyValue = _entity.getKeyValue(curve_Nurbs);
		_entity.setKeyValue(curve_Nurbs, "");
		_entity.setKeyValue(curve_CatmullRomSpline, keyValue);
	}
	else if (!m_curveCatmullRom.isEmpty() && m_curveNURBS.isEmpty()) {
		std::string keyValue = _entity.getKeyValue(curve_CatmullRomSpline);
		_entity.setKeyValue(curve_CatmullRomSpline, "");
		_entity.setKeyValue(curve_Nurbs, keyValue);
	}
}

void Doom3Group::construct()
{
	_angleObserver.setCallback(boost::bind(&RotationKey::angleChanged, &m_rotationKey, _1));
	_rotationObserver.setCallback(boost::bind(&RotationKey::rotationChanged, &m_rotationKey, _1));
	_modelObserver.setCallback(boost::bind(&Doom3Group::modelChanged, this, _1));
	_nameObserver.setCallback(boost::bind(&Doom3Group::nameChanged, this, _1));

	m_rotation.setIdentity();

	m_isModel = false;

	_owner.addKeyObserver("model", _modelObserver);
	_owner.addKeyObserver("origin", m_originKey);
	_owner.addKeyObserver("angle", _angleObserver);
	_owner.addKeyObserver("rotation", _rotationObserver);
	_owner.addKeyObserver("name", _nameObserver);
	_owner.addKeyObserver(curve_Nurbs, m_curveNURBS);
	_owner.addKeyObserver(curve_CatmullRomSpline, m_curveCatmullRom);

	updateIsModel();
}

void Doom3Group::destroy()
{
	modelChanged("");
	m_model.setActive(false); // disable callbacks during destruction

	_owner.removeKeyObserver("model", _modelObserver);
	_owner.removeKeyObserver("origin", m_originKey);
	_owner.removeKeyObserver("angle", _angleObserver);
	_owner.removeKeyObserver("rotation", _rotationObserver);
	_owner.removeKeyObserver("name", _nameObserver);
	_owner.removeKeyObserver(curve_Nurbs, m_curveNURBS);
	_owner.removeKeyObserver(curve_CatmullRomSpline, m_curveCatmullRom);
}

bool Doom3Group::isModel() const {
	return m_isModel;
}

void Doom3Group::setIsModel(bool newValue) {
	if (newValue && !m_isModel) {
		// The model key is not recognised as "name"
		m_model.modelChanged(m_modelKey);
	}
	else if (!newValue && m_isModel) {
		// Clear the model path
		m_model.modelChanged("");
		m_nameOrigin = m_origin;
	}
	m_isModel = newValue;
	updateTransform();
}

/** Determine if this Doom3Group is a model (func_static) or a
 * brush-containing entity. If the "model" key is equal to the
 * "name" key, then this is a brush-based entity, otherwise it is
 * a model entity. The exception to this is for the "worldspawn"
 * entity class, which is always a brush-based entity.
 */
void Doom3Group::updateIsModel() {
	if (m_modelKey != m_name && _entity.getKeyValue("classname") != "worldspawn") {
		setIsModel(true);

		// Set the renderable name back to 0,0,0
		_owner._renderableName.setOrigin(Vector3(0,0,0));
	}
	else {
		setIsModel(false);

		// Update the renderable name
		_owner._renderableName.setOrigin(getOrigin());
	}
}

void Doom3Group::nameChanged(const std::string& value) {
	m_name = value;
	updateIsModel();
	m_renderOrigin.updatePivot();
}

void Doom3Group::modelChanged(const std::string& value) {
	m_modelKey = value;
	updateIsModel();
	if (isModel()) {
		m_model.modelChanged(value);
		m_nameOrigin = Vector3(0,0,0);
	}
	else {
		m_model.modelChanged("");
		m_nameOrigin = m_origin;
	}
	m_renderOrigin.updatePivot();
}

void Doom3Group::updateTransform()
{
	_owner.localToParent() = Matrix4::getIdentity();

	if (isModel())
	{
		_owner.localToParent().translateBy(m_origin);
		_owner.localToParent().multiplyBy(m_rotation.getMatrix4());
	}
	
	// Notify the Node about this transformation change	to update the local2World matrix 
	_owner.transformChanged();
}

void Doom3Group::translateChildren(const Vector3& childTranslation)
{
	if (_owner.inScene())
	{
		ChildTranslator translator(childTranslation);
		_owner.traverse(translator);
	}
}

void Doom3Group::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
	// Only non-models should have their origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
		// Update the renderable name
		_owner._renderableName.setOrigin(getOrigin());
	}
	m_renderOrigin.updatePivot();
}

void Doom3Group::rotationChanged() {
	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

} // namespace entity

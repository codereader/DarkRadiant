#include "Doom3Group.h"

#include "iregistry.h"
#include "selectable.h"
#include "render.h"
#include "transformlib.h"

#include "Doom3GroupNode.h"

namespace entity {

	namespace {
		const char* RKEY_FREE_MODEL_ROTATION = "user/ui/freeModelRotation";
	}

inline void PointVertexArray_testSelect(PointVertex* first, std::size_t count, 
	SelectionTest& test, SelectionIntersection& best) 
{
	test.TestLineStrip(
	    VertexPointer(
	        reinterpret_cast<VertexPointer::pointer>(&first->vertex),
	        sizeof(PointVertex)
	    ),
	    IndexPointer::index_type(count),
	    best
	);
}

Doom3Group::Doom3Group(IEntityClassPtr eclass, 
		Doom3GroupNode& owner,
		const Callback& transformChanged, 
		const Callback& boundsChanged, 
		const Callback& evaluateTransform) :
	_owner(owner),
	_entity(_owner._entity),
	m_model(owner),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_nameOrigin(0,0,0),
	m_rotationKey(RotationChangedCaller(*this)),
	m_named(_entity),
	m_renderOrigin(m_nameOrigin),
	m_renderName(m_named, m_nameOrigin),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform),
	m_curveNURBS(boundsChanged),
	m_curveCatmullRom(boundsChanged)
{
	// construct() is called by the Doom3GroupNode
}

Doom3Group::Doom3Group(const Doom3Group& other, 
		Doom3GroupNode& owner,
		const Callback& transformChanged, 
		const Callback& boundsChanged, 
		const Callback& evaluateTransform) :
	_owner(owner),
	_entity(_owner._entity),
	m_model(owner),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(other.m_origin),
	m_nameOrigin(other.m_nameOrigin),
	m_rotationKey(RotationChangedCaller(*this)),
	m_named(_entity),
	m_renderOrigin(m_nameOrigin),
	m_renderName(m_named, m_nameOrigin),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform),
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

void Doom3Group::instanceAttach(const scene::Path& path) {
	if (++m_instanceCounter.m_count == 1) {
		_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
	}
}

void Doom3Group::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

Doom3Entity& Doom3Group::getEntity() {
	return _entity;
}
const Doom3Entity& Doom3Group::getEntity() const {
	return _entity;
}

/*Namespaced& Doom3Group::getNamespaced() {
	static NameSpaced n();
	return n;
}*/

NamedEntity& Doom3Group::getNameable() {
	return m_named;
}

const NamedEntity& Doom3Group::getNameable() const {
	return m_named;
}

TransformNode& Doom3Group::getTransformNode() {
	return m_transform;
}

const TransformNode& Doom3Group::getTransformNode() const {
	return m_transform;
}

const AABB& Doom3Group::localAABB() const {
	m_curveBounds = m_curveNURBS.getBounds();
	m_curveBounds.includeAABB(m_curveCatmullRom.getBounds());
	
	if (m_curveBounds.isValid() || !m_isModel) {
		// Include the origin as well, it might be offset
		// Only do this, if the curve has valid bounds OR we have a non-Model, 
		// otherwise we include the origin for models
		// and this AABB gets added to the children's
		// AABB in Instance::evaluateBounds(), which is wrong.
		m_curveBounds.includePoint(m_origin);
	}
	
	return m_curveBounds;
}

void Doom3Group::renderSolid(Renderer& renderer, const VolumeTest& volume, 
	const Matrix4& localToWorld, bool selected) const 
{
	if (selected) {
		m_renderOrigin.render(renderer, volume, localToWorld);
	}

	renderer.SetState(_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
	renderer.SetState(_entity.getEntityClass()->getWireShader(), Renderer::eFullMaterials);

	if (!m_curveNURBS.isEmpty()) {
		m_curveNURBS.renderSolid(renderer, volume, localToWorld);
	}
	
	if (!m_curveCatmullRom.isEmpty()) {
		m_curveCatmullRom.renderSolid(renderer, volume, localToWorld);
	}
}

void Doom3Group::renderWireframe(Renderer& renderer, const VolumeTest& volume, 
	const Matrix4& localToWorld, bool selected) const 
{
	renderSolid(renderer, volume, localToWorld, selected);

    // Render the name if required
	if (isNameVisible()) {
		renderer.addRenderable(m_renderName, localToWorld);
	}
}

void Doom3Group::testSelect(Selector& selector, SelectionTest& test, SelectionIntersection& best) {
	m_curveNURBS.testSelect(selector, test, best);
	m_curveCatmullRom.testSelect(selector, test, best);
}

void Doom3Group::snapOrigin(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&_entity);
	m_renderOrigin.updatePivot();
}

void Doom3Group::translateOrigin(const Vector3& translation) {
	m_origin = origin_translated(m_originKey.m_origin, translation);
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
	if (!rotation || (isModel() && !freeModelRotation)) {
		m_origin = origin_translated(m_originKey.m_origin, translation);
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
		rotation_rotate(m_rotation, rotation);
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
		rotation_assign(m_rotation, m_rotationKey.m_rotation);
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
		rotation_assign(m_rotationKey.m_rotation, m_rotation);
		m_rotationKey.write(&_entity, isModel());
	}
	m_curveNURBS.freezeTransform();
	m_curveNURBS.saveToEntity(_entity);
	
	m_curveCatmullRom.freezeTransform();
	m_curveCatmullRom.saveToEntity(_entity);
}

void Doom3Group::transformChanged() {
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (!isModel()) {
		ChildTransformReverter reverter;
		_owner.traverse(reverter);
		m_evaluateTransform();
	}
	else {
		// It's a model
		revertTransform();
		m_evaluateTransform();
		updateTransform();
	}
	m_curveNURBS.curveChanged();
	m_curveCatmullRom.curveChanged();
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

void Doom3Group::construct() {
	default_rotation(m_rotation);

	m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("model", Doom3Group::ModelChangedCaller(*this));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
	m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	m_keyObservers.insert("name", NameChangedCaller(*this));
	m_keyObservers.insert(curve_Nurbs, CurveNURBS::CurveChangedCaller(m_curveNURBS));
	m_keyObservers.insert(curve_CatmullRomSpline, CurveCatmullRom::CurveChangedCaller(m_curveCatmullRom));

	m_isModel = false;
	//m_nameKeys.setKeyIsName(NamespaceManager::keyIsNameDoom3Doom3Group);

	_entity.attach(m_keyObservers);
}

void Doom3Group::destroy() {
	_entity.detach(m_keyObservers);
}

void Doom3Group::addKeyObserver(const std::string& key, const KeyObserver& observer) 
{
	_entity.detach(m_keyObservers); // detach first

	m_keyObservers.insert(key, observer);

	_entity.attach(m_keyObservers); // attach again
}

void Doom3Group::removeKeyObserver(const std::string& key, const KeyObserver& observer) {
	m_keyObservers.erase(key, observer);
}

bool Doom3Group::isModel() const {
	return m_isModel;
}

void Doom3Group::setIsModel(bool newValue) {
	if (newValue && !m_isModel) {
		// The model key is not recognised as "name"
		//m_nameKeys.setKeyIsName(NamespaceManager::keyIsNameDoom3);
		m_model.modelChanged(m_modelKey);
	}
	else if (!newValue && m_isModel) {
		// The model key should be recognised as "name" (important for "namespacing")
		//m_nameKeys.setKeyIsName(NamespaceManager::keyIsNameDoom3Doom3Group);
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
	}
	else {
		setIsModel(false);
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

void Doom3Group::setTransformChanged(Callback& callback) {
	m_transformChanged = callback;
}

void Doom3Group::updateTransform() {
	m_transform.localToParent() = g_matrix4_identity;
	if (isModel()) {
		matrix4_translate_by_vec3(m_transform.localToParent(), m_origin);
		matrix4_multiply_by_matrix4(m_transform.localToParent(), rotation_toMatrix(m_rotation));
	}
	
	// Notify the Node about this transformation change	to update the local2World matrix 
	m_transformChanged();
}

void Doom3Group::translateChildren(const Vector3& childTranslation) {
	if (m_instanceCounter.m_count > 0) {
		ChildTranslator translator(childTranslation);
		_owner.traverse(translator);
	}
}

void Doom3Group::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
	// Only non-models should have their origin different than <0,0,0>
	if (!isModel()) {
		m_nameOrigin = m_origin;
	}
	m_renderOrigin.updatePivot();
}

void Doom3Group::rotationChanged() {
	rotation_assign(m_rotation, m_rotationKey.m_rotation);
	updateTransform();
}

} // namespace entity

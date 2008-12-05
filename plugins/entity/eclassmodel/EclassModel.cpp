#include "EclassModel.h"

#include "iregistry.h"
#include "EclassModelNode.h"

namespace entity {

EclassModel::EclassModel(IEntityClassPtr eclass,
						 EclassModelNode& owner, 
						 const Callback& transformChanged, 
						 const Callback& evaluateTransform) :
	_owner(owner),
	m_entity(owner._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_model(owner),
	m_named(m_entity),
	m_renderOrigin(m_origin),
	m_renderName(m_named, g_vector3_identity),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

EclassModel::EclassModel(const EclassModel& other, 
						 EclassModelNode& owner,
						 const Callback& transformChanged, 
						 const Callback& evaluateTransform) :
	_owner(owner),	
	m_entity(owner._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_model(owner),
	m_named(m_entity),
	m_renderOrigin(m_origin),
	m_renderName(m_named, g_vector3_identity),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

void EclassModel::construct() {
	default_rotation(m_rotation);

	m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
	m_keyObservers.insert("model", ModelChangedCaller(*this));
}

EclassModel::~EclassModel() {
	m_model.modelChanged("");
	m_entity.detach(m_keyObservers);
}

void EclassModel::updateTransform() {
	m_transform.localToParent() = g_matrix4_identity;
	matrix4_translate_by_vec3(m_transform.localToParent(), m_origin);

	matrix4_multiply_by_matrix4(m_transform.localToParent(), rotation_toMatrix(m_rotation));
	m_transformChanged();
}

void EclassModel::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
}

void EclassModel::angleChanged() {
	m_angle = m_angleKey.m_angle;
	updateTransform();
}

void EclassModel::rotationChanged() {
	rotation_assign(m_rotation, m_rotationKey.m_rotation);
	updateTransform();
}

void EclassModel::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attach(m_keyObservers);
		m_model.modelChanged(m_entity.getKeyValue("model"));
		_owner.skinChanged(m_entity.getKeyValue("skin"));
	}
}
	
void EclassModel::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		m_model.modelChanged("");
		m_entity.detach(m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

void EclassModel::addKeyObserver(const std::string& key, const KeyObserver& observer) {
	m_entity.detach(m_keyObservers); // detach first

	m_keyObservers.insert(key, observer);

	m_entity.attach(m_keyObservers); // attach again
}

void EclassModel::removeKeyObserver(const std::string& key, const KeyObserver& observer) {
	m_keyObservers.erase(key, observer);
}

Doom3Entity& EclassModel::getEntity() {
	return m_entity;
}
const Doom3Entity& EclassModel::getEntity() const {
	return m_entity;
}

/*Namespaced& EclassModel::getNamespaced() {
	return m_nameKeys;
}*/

NamedEntity& EclassModel::getNameable() {
	return m_named;
}

const NamedEntity& EclassModel::getNameable() const {
	return m_named;
}

TransformNode& EclassModel::getTransformNode() {
	return m_transform;
}

const TransformNode& EclassModel::getTransformNode() const {
	return m_transform;
}

void EclassModel::renderSolid(Renderer& renderer, 
	const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
{
	if(selected) {
		m_renderOrigin.render(renderer, volume, localToWorld);
	}

	renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
}
void EclassModel::renderWireframe(Renderer& renderer, 
	const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
{
	renderSolid(renderer, volume, localToWorld, selected);
	if (isNameVisible()) {
		renderer.addRenderable(m_renderName, localToWorld);
	}
}

void EclassModel::translate(const Vector3& translation) {
	m_origin = origin_translated(m_origin, translation);
}

void EclassModel::rotate(const Quaternion& rotation) {
	rotation_rotate(m_rotation, rotation);
}

void EclassModel::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void EclassModel::revertTransform() {
	m_origin = m_originKey.m_origin;
	rotation_assign(m_rotation, m_rotationKey.m_rotation);
}

void EclassModel::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);
	rotation_assign(m_rotationKey.m_rotation, m_rotation);
	m_rotationKey.write(&m_entity, true);
}

void EclassModel::transformChanged() {
	revertTransform();
	m_evaluateTransform();
	updateTransform();
}

void EclassModel::modelChanged(const std::string& value) {
	m_model.modelChanged(value);
}

} // namespace entity

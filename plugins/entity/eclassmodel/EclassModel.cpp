#include "EclassModel.h"

#include "iregistry.h"
#include "EclassModelNode.h"
#include "../EntitySettings.h"

namespace entity {

EclassModel::EclassModel(EclassModelNode& owner, 
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

void EclassModel::construct()
{
	m_rotation.setIdentity();

	m_keyObservers.insert("name", NameKey::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
	m_keyObservers.insert("model", ModelChangedCaller(*this));
}

EclassModel::~EclassModel() {
	m_model.modelChanged("");
	m_entity.detachObserver(&m_keyObservers);
}

void EclassModel::updateTransform() {
	m_transform.localToParent() = Matrix4::getIdentity();
	m_transform.localToParent().translateBy(m_origin);

	m_transform.localToParent().multiplyBy(m_rotation.getMatrix4());
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
	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

void EclassModel::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attachObserver(&m_keyObservers);
		m_model.modelChanged(m_entity.getKeyValue("model"));
		_owner.skinChanged(m_entity.getKeyValue("skin"));
	}
}
	
void EclassModel::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		m_model.modelChanged("");
		m_entity.detachObserver(&m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

void EclassModel::addKeyObserver(const std::string& key, const KeyObserver& observer) {
	m_entity.detachObserver(&m_keyObservers); // detach first

	m_keyObservers.insert(key, observer);

	m_entity.attachObserver(&m_keyObservers); // attach again
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

NameKey& EclassModel::getNameable() {
	return m_named;
}

const NameKey& EclassModel::getNameable() const {
	return m_named;
}

TransformNode& EclassModel::getTransformNode() {
	return m_transform;
}

const TransformNode& EclassModel::getTransformNode() const {
	return m_transform;
}

void EclassModel::renderSolid(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
{
	if(selected) {
		m_renderOrigin.render(collector, volume, localToWorld);
	}

	collector.SetState(m_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
}
void EclassModel::renderWireframe(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
{
	renderSolid(collector, volume, localToWorld, selected);
	if (EntitySettings::InstancePtr()->renderEntityNames()) {
		collector.addRenderable(m_renderName, localToWorld);
	}
}

void EclassModel::translate(const Vector3& translation) {
	m_origin = origin_translated(m_origin, translation);
}

void EclassModel::rotate(const Quaternion& rotation) {
	m_rotation.rotate(rotation);
}

void EclassModel::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void EclassModel::revertTransform() {
	m_origin = m_originKey.m_origin;
	m_rotation = m_rotationKey.m_rotation;
}

void EclassModel::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);
	m_rotationKey.m_rotation = m_rotation;
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

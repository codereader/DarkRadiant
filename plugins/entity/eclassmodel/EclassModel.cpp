#include "EclassModel.h"

#include "iregistry.h"
#include "EclassModelNode.h"
#include "../EntitySettings.h"

namespace entity {

EclassModel::EclassModel(EclassModelNode& owner, 
						 const Callback& transformChanged)
:	_owner(owner),
	m_entity(owner._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_model(owner),
	m_renderOrigin(m_origin),
	m_transformChanged(transformChanged)
{
	construct();
}

EclassModel::EclassModel(const EclassModel& other, 
						 EclassModelNode& owner,
						 const Callback& transformChanged)
:	_owner(owner),	
	m_entity(owner._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_model(owner),
	m_renderOrigin(m_origin),
	m_transformChanged(transformChanged)
{
	construct();
}

EclassModel::~EclassModel()
{
	destroy();
}

void EclassModel::construct()
{
	m_rotation.setIdentity();

	_owner.addKeyObserver("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	_owner.addKeyObserver("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	_owner.addKeyObserver("origin", OriginKey::OriginChangedCaller(m_originKey));
	_owner.addKeyObserver("model", ModelChangedCaller(*this));
}

void EclassModel::destroy()
{
	m_model.modelChanged("");

	_owner.removeKeyObserver("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	_owner.removeKeyObserver("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	_owner.removeKeyObserver("origin", OriginKey::OriginChangedCaller(m_originKey));
	_owner.removeKeyObserver("model", ModelChangedCaller(*this));
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
	
	assert(m_instanceCounter.m_count == 0);

	if (++m_instanceCounter.m_count == 1) {
		m_model.modelChanged(m_entity.getKeyValue("model"));
		_owner.skinChanged(m_entity.getKeyValue("skin"));
	}
}
	
void EclassModel::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		m_model.modelChanged("");
	}
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

void EclassModel::modelChanged(const std::string& value) {
	m_model.modelChanged(value);
}

} // namespace entity

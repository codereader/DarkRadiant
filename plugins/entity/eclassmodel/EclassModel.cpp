#include "EclassModel.h"

#include "iregistry.h"
#include "EclassModelNode.h"
#include "../EntitySettings.h"
#include <boost/bind.hpp>

namespace entity {

EclassModel::EclassModel(EclassModelNode& owner,
						 const Callback& transformChanged)
:	_owner(owner),
	m_entity(owner._entity),
	m_originKey(boost::bind(&EclassModel::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(boost::bind(&EclassModel::angleChanged, this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(boost::bind(&EclassModel::rotationChanged, this)),
	m_renderOrigin(m_origin),
	m_transformChanged(transformChanged)
{}

EclassModel::EclassModel(const EclassModel& other,
						 EclassModelNode& owner,
						 const Callback& transformChanged)
:	_owner(owner),
	m_entity(owner._entity),
	m_originKey(boost::bind(&EclassModel::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(boost::bind(&EclassModel::angleChanged, this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(boost::bind(&EclassModel::rotationChanged, this)),
	m_renderOrigin(m_origin),
	m_transformChanged(transformChanged)
{}

EclassModel::~EclassModel()
{
	destroy();
}

void EclassModel::construct()
{
	_rotationObserver.setCallback(boost::bind(&RotationKey::rotationChanged, &m_rotationKey, _1));
	_angleObserver.setCallback(boost::bind(&RotationKey::angleChanged, &m_rotationKey, _1));

	m_rotation.setIdentity();

	_owner.addKeyObserver("angle", _angleObserver);
	_owner.addKeyObserver("rotation", _rotationObserver);
	_owner.addKeyObserver("origin", m_originKey);
}

void EclassModel::destroy()
{
	_owner.removeKeyObserver("angle", _angleObserver);
	_owner.removeKeyObserver("rotation", _rotationObserver);
	_owner.removeKeyObserver("origin", m_originKey);
}

void EclassModel::updateTransform()
{
	_owner.localToParent() = Matrix4::getIdentity();
	_owner.localToParent().translateBy(m_origin);

	_owner.localToParent().multiplyBy(m_rotation.getMatrix4());
	m_transformChanged();
}

void EclassModel::originChanged()
{
	m_origin = m_originKey.get();
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

void EclassModel::renderSolid(RenderableCollector& collector,
	const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
{
	if (selected)
	{
		m_renderOrigin.render(collector, volume, localToWorld);
	}

	collector.SetState(_owner.getWireShader(), RenderableCollector::eWireframeOnly);
}

void EclassModel::renderWireframe(RenderableCollector& collector,
	const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
{
	renderSolid(collector, volume, localToWorld, selected);
}

void EclassModel::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	m_renderOrigin.setRenderSystem(renderSystem);
}

void EclassModel::translate(const Vector3& translation)
{
	m_origin += translation;
}

void EclassModel::rotate(const Quaternion& rotation) {
	m_rotation.rotate(rotation);
}

void EclassModel::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(m_entity);
}

void EclassModel::revertTransform()
{
	m_origin = m_originKey.get();
	m_rotation = m_rotationKey.m_rotation;
}

void EclassModel::freezeTransform()
{
	m_originKey.set(m_origin);
	m_originKey.write(m_entity);
	m_rotationKey.m_rotation = m_rotation;
	m_rotationKey.write(&m_entity, true);
}

} // namespace entity

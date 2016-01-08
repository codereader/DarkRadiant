#include "EclassModel.h"

#include "iregistry.h"
#include "EclassModelNode.h"
#include "../EntitySettings.h"
#include <functional>

namespace entity {

EclassModel::EclassModel(EclassModelNode& owner,
						 const Callback& transformChanged)
:	_owner(owner),
	m_entity(owner._entity),
	m_angleKey(std::bind(&EclassModel::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_renderOrigin(_owner._origin),
	m_transformChanged(transformChanged)
{}

EclassModel::EclassModel(const EclassModel& other,
						 EclassModelNode& owner,
						 const Callback& transformChanged)
:	_owner(owner),
	m_entity(owner._entity),
	m_angleKey(std::bind(&EclassModel::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_renderOrigin(_owner._origin),
	m_transformChanged(transformChanged)
{}

EclassModel::~EclassModel()
{
	destroy();
}

void EclassModel::construct()
{
	_rotationObserver.setCallback(std::bind(&RotationKey::rotationChanged, &_owner._rotationKey, std::placeholders::_1));
	_angleObserver.setCallback(std::bind(&RotationKey::angleChanged, &_owner._rotationKey, std::placeholders::_1));

	_owner.addKeyObserver("angle", _angleObserver);
	_owner.addKeyObserver("rotation", _rotationObserver);
}

void EclassModel::destroy()
{
	_owner.removeKeyObserver("angle", _angleObserver);
	_owner.removeKeyObserver("rotation", _rotationObserver);
}

void EclassModel::angleChanged() {
	m_angle = m_angleKey.getValue();
	_owner.updateTransform();
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
	_owner._origin += translation;
}

void EclassModel::rotate(const Quaternion& rotation) {
	_owner._rotation.rotate(rotation);
}

void EclassModel::revertTransform()
{
	_owner._origin = _owner._originKey.get();
	_owner._rotation = _owner._rotationKey.m_rotation;
}

void EclassModel::freezeTransform()
{
	_owner._originKey.set(_owner._origin);
	_owner._originKey.write(m_entity);

	_owner._rotationKey.m_rotation = _owner._rotation;
	_owner._rotationKey.write(&m_entity, true);
}

} // namespace entity

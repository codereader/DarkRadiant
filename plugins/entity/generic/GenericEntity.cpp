#include "GenericEntity.h"

#include "iregistry.h"
#include "irenderable.h"
#include "math/Frustum.h"

#include "../EntitySettings.h"
#include "GenericEntityNode.h"
#include <functional>

namespace entity {

GenericEntity::GenericEntity(GenericEntityNode& node) :
	_owner(node),
	m_entity(node._entity),
	m_originKey(std::bind(&GenericEntity::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(std::bind(&GenericEntity::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_rotationKey(std::bind(&GenericEntity::rotationChanged, this)),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	_allow3Drotations(m_entity.getKeyValue("editor_rotatable") == "1")
{}

GenericEntity::GenericEntity(const GenericEntity& other,
		GenericEntityNode& node) :
	_owner(node),
	m_entity(node._entity),
	m_originKey(std::bind(&GenericEntity::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(std::bind(&GenericEntity::angleChanged, this)),
	m_angle(AngleKey::IDENTITY),
	m_rotationKey(std::bind(&GenericEntity::rotationChanged, this)),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	_allow3Drotations(m_entity.getKeyValue("editor_rotatable") == "1")
{}

GenericEntity::~GenericEntity()
{
	destroy();
}

const AABB& GenericEntity::localAABB() const {
	return m_aabb_local;
}

void GenericEntity::renderArrow(const ShaderPtr& shader, RenderableCollector& collector,
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	if (EntitySettings::InstancePtr()->showEntityAngles())
	{
		collector.addRenderable(shader, m_arrow, localToWorld);
	}
}

void GenericEntity::renderSolid(RenderableCollector& collector,
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	// greebo: Don't render a filled cube if we have a proper model
	const ShaderPtr& shader = _owner.getSolidAABBRenderMode() == GenericEntityNode::WireFrameOnly ?
		_owner.getWireShader() : _owner.getFillShader();

	collector.addRenderable(shader, m_aabb_solid, localToWorld);
	renderArrow(shader, collector, volume, localToWorld);
}

void GenericEntity::renderWireframe(RenderableCollector& collector,
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	collector.addRenderable(_owner.getWireShader(), m_aabb_wire, localToWorld);
	renderArrow(_owner.getWireShader(), collector, volume, localToWorld);
}

void GenericEntity::testSelect(Selector& selector,
	SelectionTest& test, const Matrix4& localToWorld)
{
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	aabb_testselect(m_aabb_local, test, best);
	if(best.isValid()) {
		selector.addIntersection(best);
	}
}

void GenericEntity::translate(const Vector3& translation)
{
	m_origin += translation;
}

void GenericEntity::rotate(const Quaternion& rotation)
{
	if (_allow3Drotations)
	{
        m_rotation.rotate(rotation);
	}
	else
	{
		m_angle = AngleKey::getRotatedValue(m_angle, rotation);
	}
}

void GenericEntity::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(m_entity);
}

void GenericEntity::revertTransform()
{
	m_origin = m_originKey.get();

	if (_allow3Drotations)
	{
		m_rotation = m_rotationKey.m_rotation;
	}
	else
	{
		m_angle = m_angleKey.getValue();
	}
}

void GenericEntity::freezeTransform()
{
	m_originKey.set(m_origin);
	m_originKey.write(m_entity);

	if (_allow3Drotations)
	{
		m_rotationKey.m_rotation = m_rotation;
		m_rotationKey.m_rotation.writeToEntity(&m_entity);
	}
	else
	{
		m_angleKey.setValue(m_angle);
		m_angleKey.write(&m_entity);
	}
}

void GenericEntity::construct()
{
	m_aabb_local = m_entity.getEntityClass()->getBounds();
	m_ray.origin = m_aabb_local.getOrigin();
	m_ray.direction = Vector3(1, 0, 0);
	m_rotation.setIdentity();

	if (!_allow3Drotations)
	{
		_angleObserver.setCallback(std::bind(&AngleKey::angleChanged, &m_angleKey, std::placeholders::_1));

		// Ordinary rotation (2D around z axis), use angle key observer
		_owner.addKeyObserver("angle", _angleObserver);
	}
	else
	{
		_angleObserver.setCallback(std::bind(&RotationKey::angleChanged, &m_rotationKey, std::placeholders::_1));
		_rotationObserver.setCallback(std::bind(&RotationKey::rotationChanged, &m_rotationKey, std::placeholders::_1));

		// Full 3D rotations allowed, observe both keys using the rotation key observer
		_owner.addKeyObserver("angle", _angleObserver);
		_owner.addKeyObserver("rotation", _rotationObserver);
	}

	_owner.addKeyObserver("origin", m_originKey);
}

void GenericEntity::destroy()
{
	if (!_allow3Drotations)
	{
		// Ordinary rotation (2D around z axis), use angle key observer
		_owner.removeKeyObserver("angle", _angleObserver);
	}
	else
	{
		// Full 3D rotations allowed, observe both keys using the rotation key observer
		_owner.removeKeyObserver("angle", _angleObserver);
		_owner.removeKeyObserver("rotation", _rotationObserver);
	}

	_owner.removeKeyObserver("origin", m_originKey);
}

void GenericEntity::updateTransform()
{
	_owner.localToParent() = Matrix4::getTranslation(m_origin);

	if (_allow3Drotations)
	{
		// greebo: Use the z-direction as base for rotations
		m_ray.direction = m_rotation.getMatrix4().transformDirection(Vector3(0,0,1));
	}
	else
	{
		m_ray.direction = Matrix4::getRotationAboutZDegrees(m_angle).transformDirection(Vector3(1, 0, 0));
	}

	_owner.transformChanged();
}

void GenericEntity::originChanged()
{
	m_origin = m_originKey.get();
	updateTransform();
}

void GenericEntity::angleChanged()
{
	// Ignore the angle key when 3D rotations are enabled
	if (_allow3Drotations) return;

	m_angle = m_angleKey.getValue();
	updateTransform();
}

void GenericEntity::rotationChanged()
{
	// Ignore the rotation key, when in 2D "angle" mode
	if (!_allow3Drotations) return;

	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

const Vector3& GenericEntity::getDirection() const
{
	return m_ray.direction;
}

const Vector3& GenericEntity::getUntransformedOrigin() const
{
    return m_originKey.get();
}

} // namespace entity

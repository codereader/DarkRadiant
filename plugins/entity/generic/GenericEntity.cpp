#include "GenericEntity.h"

#include "iregistry.h"
#include "irenderable.h"
#include "math/frustum.h"

#include "../EntitySettings.h"
#include "GenericEntityNode.h"

namespace entity {

GenericEntity::GenericEntity(GenericEntityNode& node, 
		const Callback& transformChanged) :
	_owner(node),
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_transformChanged(transformChanged),
	_allow3Drotations(m_entity.getKeyValue("editor_rotatable") == "1")
{}

GenericEntity::GenericEntity(const GenericEntity& other, 
		GenericEntityNode& node, 
		const Callback& transformChanged) :
	_owner(node),
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_rotationKey(RotationChangedCaller(*this)),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_transformChanged(transformChanged),
	_allow3Drotations(m_entity.getKeyValue("editor_rotatable") == "1")
{}

GenericEntity::~GenericEntity()
{
	destroy();
}

TransformNode& GenericEntity::getTransformNode() {
	return m_transform;
}

const TransformNode& GenericEntity::getTransformNode() const {
	return m_transform;
}

const AABB& GenericEntity::localAABB() const {
	return m_aabb_local;
}

VolumeIntersectionValue GenericEntity::intersectVolume(
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	return volume.TestAABB(localAABB(), localToWorld);
}

void GenericEntity::renderArrow(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	if (GlobalRegistry().get("user/ui/xyview/showEntityAngles") == "1") {
		collector.addRenderable(m_arrow, localToWorld);
	}
}

void GenericEntity::renderSolid(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	collector.SetState(m_entity.getEntityClass()->getFillShader(), RenderableCollector::eFullMaterials);
	collector.addRenderable(m_aabb_solid, localToWorld);
	renderArrow(collector, volume, localToWorld);
}

void GenericEntity::renderWireframe(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	collector.SetState(m_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
	collector.addRenderable(m_aabb_wire, localToWorld);
	renderArrow(collector, volume, localToWorld);
}

void GenericEntity::testSelect(Selector& selector, 
	SelectionTest& test, const Matrix4& localToWorld)
{
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	aabb_testselect(m_aabb_local, test, best);
	if(best.valid()) {
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
		// greebo: Pre-multiply the incoming matrix on the existing one
		// Don't use m_rotation.rotate(), which performs a post-multiplication
		m_rotation.setFromMatrix4( 
			matrix4_premultiplied_by_matrix4(
				m_rotation.getMatrix4(), 
				matrix4_rotation_for_quaternion_quantised(rotation)
			)
		);
	}
	else
	{
		m_angle = angle_rotated(m_angle, rotation);
	}
}

void GenericEntity::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void GenericEntity::revertTransform()
{
	m_origin = m_originKey.m_origin;

	if (_allow3Drotations)
	{
		m_rotation = m_rotationKey.m_rotation;
	}
	else
	{
		m_angle = m_angleKey.m_angle;
	}
}

void GenericEntity::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);

	if (_allow3Drotations)
	{
		m_rotationKey.m_rotation = m_rotation;
		m_rotationKey.m_rotation.writeToEntity(&m_entity);
	}
	else
	{
		m_angleKey.m_angle = m_angle;
		m_angleKey.write(&m_entity);
	}
}

void GenericEntity::construct() {
	m_aabb_local = m_entity.getEntityClass()->getBounds();
	m_ray.origin = m_aabb_local.getOrigin();
	m_ray.direction = Vector3(1, 0, 0);
	m_rotation.setIdentity();

	if (!_allow3Drotations)
	{
		// Ordinary rotation (2D around z axis), use angle key observer
		_owner.addKeyObserver("angle", AngleKey::AngleChangedCaller(m_angleKey));
	}
	else
	{
		// Full 3D rotations allowed, observe both keys using the rotation key observer
		_owner.addKeyObserver("angle", RotationKey::AngleChangedCaller(m_rotationKey));
		_owner.addKeyObserver("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	}

	_owner.addKeyObserver("origin", OriginKey::OriginChangedCaller(m_originKey));
}

void GenericEntity::destroy()
{
	if (!_allow3Drotations)
	{
		// Ordinary rotation (2D around z axis), use angle key observer
		_owner.removeKeyObserver("angle", AngleKey::AngleChangedCaller(m_angleKey));
	}
	else
	{
		// Full 3D rotations allowed, observe both keys using the rotation key observer
		_owner.removeKeyObserver("angle", RotationKey::AngleChangedCaller(m_rotationKey));
		_owner.removeKeyObserver("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	}

	_owner.removeKeyObserver("origin", OriginKey::OriginChangedCaller(m_originKey));
}

void GenericEntity::updateTransform()
{
	m_transform.localToParent() = Matrix4::getTranslation(m_origin);

	if (_allow3Drotations)
	{
		// greebo: Use the z-direction as base for rotations
		m_ray.direction = matrix4_transformed_direction(m_rotation.getMatrix4(), Vector3(0,0,1));
	}
	else
	{
		m_ray.direction = matrix4_transformed_direction(matrix4_rotation_for_z(degrees_to_radians(m_angle)), Vector3(1, 0, 0));
	}

	m_transformChanged();
}

void GenericEntity::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
}

void GenericEntity::angleChanged()
{
	// Ignore the angle key when 3D rotations are enabled
	if (_allow3Drotations) return;

	m_angle = m_angleKey.m_angle;
	updateTransform();
}

void GenericEntity::rotationChanged()
{
	// Ignore the rotation key, when in 2D "angle" mode
	if (!_allow3Drotations) return;

	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

} // namespace entity

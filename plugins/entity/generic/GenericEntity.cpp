#include "GenericEntity.h"

#include "iregistry.h"
#include "irenderable.h"
#include "math/frustum.h"

#include "GenericEntityNode.h"

namespace entity {

GenericEntity::GenericEntity(IEntityClassPtr eclass, 
		GenericEntityNode& node, 
		const Callback& transformChanged, 
		const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_named(m_entity),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_renderName(m_named, g_vector3_identity),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

GenericEntity::GenericEntity(const GenericEntity& other, 
		GenericEntityNode& node, 
		const Callback& transformChanged, 
		const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_named(m_entity),
	m_arrow(m_ray),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_renderName(m_named, g_vector3_identity),
	m_transformChanged(transformChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

void GenericEntity::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attach(m_keyObservers);
	}
}

void GenericEntity::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		m_entity.detach(m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

Doom3Entity& GenericEntity::getEntity() {
	return m_entity;
}
const Doom3Entity& GenericEntity::getEntity() const {
	return m_entity;
}

/*Namespaced& GenericEntity::getNamespaced() {
	return m_nameKeys;
}*/

NamedEntity& GenericEntity::getNameable() {
	return m_named;
}

const NamedEntity& GenericEntity::getNameable() const {
	return m_named;
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

void GenericEntity::renderArrow(Renderer& renderer, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	if (GlobalRegistry().get("user/ui/xyview/showEntityAngles") == "1") {
		renderer.addRenderable(m_arrow, localToWorld);
	}
}

void GenericEntity::renderSolid(Renderer& renderer, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	renderer.SetState(m_entity.getEntityClass()->getFillShader(), Renderer::eFullMaterials);
	renderer.addRenderable(m_aabb_solid, localToWorld);
	renderArrow(renderer, volume, localToWorld);
}

void GenericEntity::renderWireframe(Renderer& renderer, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
	renderer.addRenderable(m_aabb_wire, localToWorld);
	renderArrow(renderer, volume, localToWorld);
	
	if (isNameVisible()) {
		renderer.addRenderable(m_renderName, localToWorld);
	}
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

void GenericEntity::translate(const Vector3& translation) {
	m_origin = origin_translated(m_origin, translation);
}

void GenericEntity::rotate(const Quaternion& rotation) {
	m_angle = angle_rotated(m_angle, rotation);
}

void GenericEntity::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void GenericEntity::revertTransform() {
	m_origin = m_originKey.m_origin;
	m_angle = m_angleKey.m_angle;
}

void GenericEntity::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);
	m_angleKey.m_angle = m_angle;
	m_angleKey.write(&m_entity);
}

void GenericEntity::transformChanged() {
	revertTransform();
	m_evaluateTransform();
	updateTransform();
}

void GenericEntity::construct() {
	m_aabb_local = m_entity.getEntityClass()->getBounds();
	m_ray.origin = m_aabb_local.getOrigin();
	m_ray.direction[0] = 1;
	m_ray.direction[1] = 0;
	m_ray.direction[2] = 0;

	m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("angle", AngleKey::AngleChangedCaller(m_angleKey));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
}

void GenericEntity::updateTransform() {
	m_transform.localToParent() = g_matrix4_identity;
	matrix4_translate_by_vec3(m_transform.localToParent(), m_origin);
	m_ray.direction = matrix4_transformed_direction(matrix4_rotation_for_z(degrees_to_radians(m_angle)), Vector3(1, 0, 0));
	m_transformChanged();
}

void GenericEntity::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
}

void GenericEntity::angleChanged() {
	m_angle = m_angleKey.m_angle;
	updateTransform();
}

} // namespace entity

#include "Speaker.h"

#include "iregistry.h"
#include "irenderable.h"
#include "isound.h"
#include <stdlib.h>
#include "SpeakerNode.h"
#include "../EntitySettings.h"

namespace entity {

Speaker::Speaker(SpeakerNode& node, 
		const Callback& transformChanged, 
		const Callback& boundsChanged,
		const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_named(m_entity),
	_renderableRadii(m_aabb_local.origin),
	m_useSpeakerRadii(true),
	m_minIsSet(false),
	m_maxIsSet(false),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_renderName(m_named, g_vector3_identity),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

Speaker::Speaker(const Speaker& other, 
		SpeakerNode& node, 
		const Callback& transformChanged, 
		const Callback& boundsChanged,
		const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_angleKey(AngleChangedCaller(*this)),
	m_angle(ANGLEKEY_IDENTITY),
	m_named(m_entity),
	_renderableRadii(m_aabb_local.origin),
	m_useSpeakerRadii(true),
	m_minIsSet(false),
	m_maxIsSet(false),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_renderName(m_named, g_vector3_identity),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

void Speaker::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attach(m_keyObservers);
	}
}

void Speaker::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		m_entity.detach(m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

Doom3Entity& Speaker::getEntity() {
	return m_entity;
}
const Doom3Entity& Speaker::getEntity() const {
	return m_entity;
}

/*Namespaced& Speaker::getNamespaced() {
	return m_nameKeys;
}*/

NamedEntity& Speaker::getNameable() {
	return m_named;
}

const NamedEntity& Speaker::getNameable() const {
	return m_named;
}

TransformNode& Speaker::getTransformNode() {
	return m_transform;
}

const TransformNode& Speaker::getTransformNode() const {
	return m_transform;
}

const AABB& Speaker::localAABB() const {
	return m_aabb_border;
}

VolumeIntersectionValue Speaker::intersectVolume(
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	return volume.TestAABB(localAABB(), localToWorld);
}

// Submit renderables for solid mode
void Speaker::renderSolid(RenderableCollector& collector, 
                          const VolumeTest& volume,
                          const Matrix4& localToWorld,
                          bool isSelected) const
{
	collector.SetState(m_entity.getEntityClass()->getFillShader(), RenderableCollector::eFullMaterials);
	collector.addRenderable(m_aabb_solid, localToWorld);

    // Submit the speaker radius if we are selected or the "show all speaker
    // radii" option is set
	if (isSelected || EntitySettings::InstancePtr()->showAllSpeakerRadii())
    {
		collector.addRenderable(_renderableRadii, localToWorld);
    }
}

// Submit renderables for wireframe mode
void Speaker::renderWireframe(RenderableCollector& collector, 
                              const VolumeTest& volume,
                              const Matrix4& localToWorld,
                              bool isSelected) const
{
	collector.SetState(m_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
	collector.addRenderable(m_aabb_wire, localToWorld);

    // Submit the speaker radius if we are selected or the "show all speaker
    // radii" option is set
	if (isSelected || EntitySettings::InstancePtr()->showAllSpeakerRadii())
    {
		collector.addRenderable(_renderableRadii, localToWorld);
    }
	
    // Submit renderable text name if required
	if (EntitySettings::InstancePtr()->renderEntityNames()) 
    {
		collector.addRenderable(m_renderName, localToWorld);
	}
}

void Speaker::testSelect(Selector& selector, 
	SelectionTest& test, const Matrix4& localToWorld)
{
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	aabb_testselect(m_aabb_local, test, best);
	if(best.valid()) {
		selector.addIntersection(best);
	}
}

void Speaker::translate(const Vector3& translation) {
	m_origin = origin_translated(m_origin, translation);
}

void Speaker::rotate(const Quaternion& rotation) {
	m_angle = angle_rotated(m_angle, rotation);
}

void Speaker::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void Speaker::revertTransform() {
	m_origin = m_originKey.m_origin;
	m_angle = m_angleKey.m_angle;
}

void Speaker::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);
	m_angleKey.m_angle = m_angle;
	m_angleKey.write(&m_entity);
}

void Speaker::transformChanged() {
	revertTransform();
	m_evaluateTransform();
	updateTransform();
}

void Speaker::construct() {
	m_aabb_local = m_entity.getEntityClass()->getBounds();
	m_aabb_border = m_aabb_local;
	
	m_ray.origin = m_aabb_local.getOrigin();
	m_ray.direction[0] = 1;
	m_ray.direction[1] = 0;
	m_ray.direction[2] = 0;

	m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("angle", AngleKey::AngleChangedCaller(m_angleKey));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
	m_keyObservers.insert("s_shader", Speaker::sShaderChangedCaller(*this));
	m_keyObservers.insert("s_mindistance", Speaker::sMinChangedCaller(*this));
	m_keyObservers.insert("s_maxdistance", Speaker::sMaxChangedCaller(*this));
}

void Speaker::updateAABB() {
	// set the AABB to the biggest AABB the speaker contains
	m_aabb_border = m_aabb_local;
	m_aabb_border.includeAABB(_renderableRadii.localAABB());
	m_boundsChanged();
}

void Speaker::updateTransform() {
	m_transform.localToParent() = Matrix4::getIdentity();
	m_transform.localToParent().translateBy(m_origin);
	m_ray.direction = matrix4_transformed_direction(matrix4_rotation_for_z(degrees_to_radians(m_angle)), Vector3(1, 0, 0));
	m_transformChanged();
}

void Speaker::originChanged() {
	m_origin = m_originKey.m_origin;
	updateTransform();
}

void Speaker::angleChanged() {
	m_angle = m_angleKey.m_angle;
	updateTransform();
}

void Speaker::sShaderChanged(const std::string& value) {
	if (value.empty()) {
		m_stdVal.setMin(0);
		m_stdVal.setMax(0);
	}
	else {
		m_stdVal = GlobalSoundManager().getSoundShader(value)->getRadii();
	}
	if (!m_minIsSet) _renderableRadii.setMin(m_stdVal.getMin());
	if (!m_maxIsSet) _renderableRadii.setMax(m_stdVal.getMax());

	updateAABB();
}

void Speaker::sMinChanged(const std::string& value) {
	m_minIsSet = value.empty() ? false : true;
	if (m_minIsSet)
		// we need to parse in metres
		_renderableRadii.setMin(strToFloat(value), true);
	else 
		_renderableRadii.setMin(m_stdVal.getMin());

	updateAABB();
}

void Speaker::sMaxChanged(const std::string& value) {
	m_maxIsSet = value.empty() ? false : true;
	if (m_maxIsSet)
		// we need to parse in metres
		_renderableRadii.setMax(strToFloat(value), true);
	else 
		_renderableRadii.setMax(m_stdVal.getMax());

	updateAABB();
}

} // namespace entity

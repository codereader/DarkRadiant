#include "Speaker.h"

#include "itextstream.h"
#include "iregistry.h"
#include "irenderable.h"
#include "isound.h"
#include <stdlib.h>
#include "SpeakerNode.h"
#include "../EntitySettings.h"

namespace entity {

	namespace 
	{
		const std::string KEY_S_MAXDISTANCE("s_maxdistance");
		const std::string KEY_S_MINDISTANCE("s_mindistance");
		const std::string KEY_S_SHADER("s_shader");
	}

Speaker::Speaker(SpeakerNode& node, 
		const Callback& transformChanged, 
		const Callback& boundsChanged,
		const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_named(m_entity),
	_renderableRadii(m_origin, _radiiTransformed),
	m_useSpeakerRadii(true),
	m_minIsSet(false),
	m_maxIsSet(false),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
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
	m_named(m_entity),
	_renderableRadii(m_origin, _radiiTransformed),
	m_useSpeakerRadii(true),
	m_minIsSet(false),
	m_maxIsSet(false),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

void Speaker::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attachObserver(&m_keyObservers);
	}
}

void Speaker::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		m_entity.detachObserver(&m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

Doom3Entity& Speaker::getEntity() {
	return m_entity;
}
const Doom3Entity& Speaker::getEntity() const {
	return m_entity;
}

NameKey& Speaker::getNameable() {
	return m_named;
}

const NameKey& Speaker::getNameable() const {
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

void Speaker::translate(const Vector3& translation)
{
	m_origin = origin_translated(m_origin, translation);
}

void Speaker::rotate(const Quaternion& rotation)
{
	// nothing to rotate here, speakers are symmetric
}

void Speaker::snapto(float snap) {
	m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
	m_originKey.write(&m_entity);
}

void Speaker::revertTransform()
{
	m_origin = m_originKey.m_origin;

	_radiiTransformed = _radii;
}

void Speaker::freezeTransform() {
	m_originKey.m_origin = m_origin;
	m_originKey.write(&m_entity);

	_radii = _radiiTransformed;

	// Write the s_mindistance/s_maxdistance keyvalues if we have a valid shader
	if (!m_entity.getKeyValue(KEY_S_SHADER).empty())
	{
		// Note: Write the spawnargs in meters

		if (_radii.getMax() != _defaultRadii.getMax())
		{
			m_entity.setKeyValue(KEY_S_MAXDISTANCE, floatToStr(_radii.getMax(true)));
		}
		else
		{
			// Radius is matching default, clear the spawnarg
			m_entity.setKeyValue(KEY_S_MAXDISTANCE, "");
		}

		if (_radii.getMin() != _defaultRadii.getMin())
		{
			m_entity.setKeyValue(KEY_S_MINDISTANCE, floatToStr(_radii.getMin(true)));
		}
		else
		{
			// Radius is matching default, clear the spawnarg
			m_entity.setKeyValue(KEY_S_MINDISTANCE, "");
		}
	}
}

void Speaker::transformChanged() {
	revertTransform();
	m_evaluateTransform();
	updateTransform();
}

void Speaker::setRadiusFromAABB(const AABB& aabb)
{
	// Find out which dimension got changed the most
	Vector3 delta = aabb.getExtents() - localAABB().getExtents();

	double maxTrans;

	// Get the maximum translation component
	if (fabs(delta.x()) > fabs(delta.y()))
	{
		maxTrans = (fabs(delta.x()) > fabs(delta.z())) ? delta.x() : delta.z();
	}
	else
	{
		maxTrans = (fabs(delta.y()) > fabs(delta.z())) ? delta.y() : delta.z();
	}

	if (EntitySettings::InstancePtr()->dragResizeEntitiesSymmetrically())
	{
		// For a symmetric AABB change, take the extents delta times 2
		maxTrans *= 2;
	}
	else
	{
		// Update the origin accordingly
		m_origin += aabb.origin - localAABB().getOrigin();
	}

	float oldRadius = _radii.getMax();
	float newRadius = static_cast<float>(oldRadius + maxTrans);

	float ratio = newRadius / oldRadius;

	// Resize the radii and update the min radius proportionally
	_radiiTransformed.setMax(newRadius);
	_radiiTransformed.setMin(_radii.getMin() * ratio);

	updateAABB();
	updateTransform();
}

void Speaker::construct() {
	m_aabb_local = m_entity.getEntityClass()->getBounds();
	m_aabb_border = m_aabb_local;
	
	m_keyObservers.insert("name", NameKey::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
	m_keyObservers.insert(KEY_S_SHADER, Speaker::sShaderChangedCaller(*this));
	m_keyObservers.insert(KEY_S_MINDISTANCE, Speaker::sMinChangedCaller(*this));
	m_keyObservers.insert(KEY_S_MAXDISTANCE, Speaker::sMaxChangedCaller(*this));
}

void Speaker::updateAABB()
{
	// set the AABB to the biggest AABB the speaker contains
	m_aabb_border = m_aabb_local;

	float radius = _radiiTransformed.getMax();
	m_aabb_border.extents = Vector3(radius, radius, radius);

	m_boundsChanged();
}

void Speaker::updateTransform()
{
	m_transform.localToParent() = Matrix4::getTranslation(m_origin);
	m_transformChanged();
}

void Speaker::originChanged()
{
	m_origin = m_originKey.m_origin;
	updateTransform();
}

void Speaker::sShaderChanged(const std::string& value)
{
	if (value.empty())
	{
		_defaultRadii.setMin(0);
		_defaultRadii.setMax(0);
	}
	else
	{
		// Non-zero shader set, retrieve the default radii
		_defaultRadii = GlobalSoundManager().getSoundShader(value)->getRadii();
	}

	// If we haven't overridden our distances yet, adjust these values to defaults
	if (!m_minIsSet)
	{
		_radii.setMin(_defaultRadii.getMin());
	}

	if (!m_maxIsSet) 
	{
		_radii.setMax(_defaultRadii.getMax());
	}

	// Store the new values into our working set
	_radiiTransformed = _radii;

	updateAABB();
}

void Speaker::sMinChanged(const std::string& value)
{
	// Check whether the spawnarg got set or removed
	m_minIsSet = value.empty() ? false : true;

	if (m_minIsSet)
	{
		// we need to parse in metres
		_radii.setMin(strToFloat(value), true);
	}
	else 
	{
		_radii.setMin(_defaultRadii.getMin());
	}

	// Store the new value into our working set
	_radiiTransformed.setMin(_radii.getMin());

	updateAABB();
}

void Speaker::sMaxChanged(const std::string& value)
{
	m_maxIsSet = value.empty() ? false : true;

	if (m_maxIsSet)
	{
		// we need to parse in metres
		_radii.setMax(strToFloat(value), true);
	}
	else 
	{
		_radii.setMax(_defaultRadii.getMax());
	}

	// Store the new value into our working set
	_radiiTransformed.setMax(_radii.getMax());

	updateAABB();
}

} // namespace entity

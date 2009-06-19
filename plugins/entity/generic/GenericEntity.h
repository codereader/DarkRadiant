#ifndef GENERICENTITY_H_
#define GENERICENTITY_H_

#include "Bounded.h"
#include "cullable.h"
#include "editable.h"

#include "math/Vector3.h"
#include "entitylib.h"
#include "generic/callback.h"

#include "../origin.h"
#include "../angle.h"
#include "../rotation.h"
#include "../namedentity.h"
#include "../keyobservers.h"
#include "../Doom3Entity.h"

#include "RenderableArrow.h"

namespace entity {

class GenericEntityNode;

class GenericEntity :
	public Cullable,
	public Bounded,
	public Snappable
{
	Doom3Entity& m_entity;
	KeyObserverMap m_keyObservers;
	MatrixTransform m_transform;

	OriginKey m_originKey;
	Vector3 m_origin;

	// The AngleKey wraps around the "angle" spawnarg
	AngleKey m_angleKey;

	// This is the "working copy" of the angle value
	float m_angle;

	// The RotationKey takes care of the "rotation" spawnarg
	RotationKey m_rotationKey;

	// This is the "working copy" of the rotation value
	Float9 m_rotation;

	NamedEntity m_named;

	AABB m_aabb_local;
	Ray m_ray;

	RenderableArrow m_arrow;
	RenderableSolidAABB m_aabb_solid;
	RenderableWireframeAABB m_aabb_wire;
	RenderableNamedEntity m_renderName;

	Callback m_transformChanged;
	Callback m_evaluateTransform;

	// TRUE if this entity's arrow can be rotated in all directions, 
	// FALSE if the arrow is caught in the xy plane
	bool _allow3Drotations;
public:
	// Constructor
	GenericEntity(GenericEntityNode& node, 
				  const Callback& transformChanged, 
				  const Callback& evaluateTransform);
	
	// Copy constructor
	GenericEntity(const GenericEntity& other, 
				  GenericEntityNode& node, 
				  const Callback& transformChanged, 
				  const Callback& evaluateTransform);

	InstanceCounter m_instanceCounter;
	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	Doom3Entity& getEntity();
	const Doom3Entity& getEntity() const;

	NamedEntity& getNameable();
	const NamedEntity& getNameable() const;
	TransformNode& getTransformNode();
	const TransformNode& getTransformNode() const;

	const AABB& localAABB() const;

	VolumeIntersectionValue intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;

	void renderArrow(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	
	void snapto(float snap);
	
	void revertTransform();
	void freezeTransform();
	void transformChanged();
	typedef MemberCaller<GenericEntity, &GenericEntity::transformChanged> TransformChangedCaller;

public:

	void construct();

	void updateTransform();
	typedef MemberCaller<GenericEntity, &GenericEntity::updateTransform> UpdateTransformCaller;
	
	void originChanged();
	typedef MemberCaller<GenericEntity, &GenericEntity::originChanged> OriginChangedCaller;
	
	void angleChanged();
	typedef MemberCaller<GenericEntity, &GenericEntity::angleChanged> AngleChangedCaller;

	void rotationChanged();
	typedef MemberCaller<GenericEntity, &GenericEntity::rotationChanged> RotationChangedCaller;
};

} // namespace entity

#endif /*GENERICENTITY_H_*/

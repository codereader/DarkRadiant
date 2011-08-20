#pragma once

#include "Bounded.h"
#include "editable.h"

#include "math/Vector3.h"
#include "entitylib.h"

#include "../OriginKey.h"
#include "../angle.h"
#include "../rotation.h"
#include "../Doom3Entity.h"
#include "../KeyObserverDelegate.h"
#include "transformlib.h"

#include "RenderableArrow.h"

namespace entity {

class GenericEntityNode;

class GenericEntity :
	public Bounded,
	public Snappable
{
private:
	GenericEntityNode& _owner;

	Doom3Entity& m_entity;

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

	AABB m_aabb_local;
	Ray m_ray;

	RenderableArrow m_arrow;
	RenderableSolidAABB m_aabb_solid;
	RenderableWireframeAABB m_aabb_wire;

	// TRUE if this entity's arrow can be rotated in all directions,
	// FALSE if the arrow is caught in the xy plane
	bool _allow3Drotations;

	KeyObserverDelegate _rotationObserver;
	KeyObserverDelegate _angleObserver;

public:
	// Constructor
	GenericEntity(GenericEntityNode& node);

	// Copy constructor
	GenericEntity(const GenericEntity& other,
				  GenericEntityNode& node);

	~GenericEntity();

	const AABB& localAABB() const;

	void renderArrow(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);

	void snapto(float snap);

	void revertTransform();
	void freezeTransform();

	const Vector3& getDirection() const;

public:

	void construct();
	void destroy();

	void updateTransform();

	void originChanged();
	void angleChanged();
	void rotationChanged();
};

} // namespace entity

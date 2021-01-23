#pragma once

#include "editable.h"
#include "inamespace.h"

#include "selectionlib.h"
#include "transformlib.h"
#include "irenderable.h"
#include "math/Ray.h"

#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../OriginKey.h"
#include "../AngleKey.h"
#include "../RotationKey.h"
#include "../SpawnArgs.h"
#include "../KeyObserverDelegate.h"

#include "RenderableArrow.h"

namespace entity
{

class GenericEntityNode;
typedef std::shared_ptr<GenericEntityNode> GenericEntityNodePtr;

/// EntityNode subclass for all entity types not handled by a specific class
class GenericEntityNode: public EntityNode, public Snappable
{
	OriginKey m_originKey;
	Vector3 m_origin;

	// The AngleKey wraps around the "angle" spawnarg
	AngleKey m_angleKey;

	// This is the "working copy" of the angle value
	float m_angle;

	// The RotationKey takes care of the "rotation" spawnarg
	RotationKey m_rotationKey;

	// This is the "working copy" of the rotation value
	RotationMatrix m_rotation;

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

    // Whether to draw a solid/shaded box in full material render mode or just the wireframe
    enum SolidAAABBRenderMode
    {
        SolidBoxes,
        WireFrameOnly,
    };

    SolidAAABBRenderMode _solidAABBRenderMode;

public:
	GenericEntityNode(const IEntityClassPtr& eclass);
    ~GenericEntityNode();

private:
	GenericEntityNode(const GenericEntityNode& other);

	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);

	void revertTransform() override;
	void freezeTransform() override;
	void updateTransform();

	void originChanged();
	void angleChanged();
	void rotationChanged();

public:
	static GenericEntityNodePtr Create(const IEntityClassPtr& eclass);

	// Snappable implementation
	void snapto(float snap) override;

	// Bounded implementation
	const AABB& localAABB() const override;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test) override;

	scene::INodePtr clone() const override;

	// Renderable implementation
	void renderArrow(const ShaderPtr& shader, RenderableCollector& collector,
                     const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;

    SolidAAABBRenderMode getSolidAABBRenderMode() const;

	// Override EntityNode::getDirection()
	const Vector3& getDirection() const override;

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

    void onChildAdded(const scene::INodePtr& child) override;
	void onChildRemoved(const scene::INodePtr& child) override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Override EntityNode::construct()
	void construct() override;
};

} // namespace entity

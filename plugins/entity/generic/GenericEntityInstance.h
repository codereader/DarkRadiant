#ifndef GENERICENTITYINSTANCE_H_
#define GENERICENTITYINSTANCE_H_

#include "Bounded.h"
#include "cullable.h"
#include "selectable.h"
#include "renderable.h"

#include "generic/callbackfwd.h"
#include "math/aabb.h"
#include "math/frustum.h"
#include "scenelib.h"
#include "transformlib.h"

#include "../target/TargetableInstance.h"

#include "GenericEntity.h"

namespace entity {

class GenericEntityInstance :
	public TargetableInstance,
	public TransformModifier,
	public Renderable,
	public SelectionTestable,
	public Bounded,
	public Cullable
{
	GenericEntity& m_contained;
	mutable AABB m_bounds;

public:
	GenericEntityInstance(const scene::Path& path, scene::Instance* parent, GenericEntity& contained);
	~GenericEntityInstance();

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(
	    const VolumeTest& test, const Matrix4& localToWorld) const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	void testSelect(Selector& selector, SelectionTest& test);

	void evaluateTransform();
	
	void applyTransform();
	typedef MemberCaller<GenericEntityInstance, &GenericEntityInstance::applyTransform> ApplyTransformCaller;
};

} // namespace entity

#endif /*GENERICENTITYINSTANCE_H_*/

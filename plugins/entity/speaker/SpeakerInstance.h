#ifndef SPEAKERINSTANCE_H_
#define SPEAKERINSTANCE_H_

#include "Bounded.h"
#include "cullable.h"
#include "selectable.h"
#include "renderable.h"

#include "generic/callbackfwd.h"
#include "math/aabb.h"
#include "math/frustum.h"
#include "scenelib.h"
#include "transformlib.h"

#include "../targetable.h"

#include "Speaker.h"

namespace entity {

class SpeakerInstance :
	public TargetableInstance,
	public TransformModifier,
	public Renderable,
	public SelectionTestable,
	public Bounded,
	public Cullable
{
	Speaker& m_contained;
	mutable AABB m_bounds;

public:
	SpeakerInstance(const scene::Path& path, scene::Instance* parent, Speaker& contained);
	~SpeakerInstance();

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
	typedef MemberCaller<SpeakerInstance, &SpeakerInstance::applyTransform> ApplyTransformCaller;
};

} // namespace entity

#endif /*SPEAKERINSTANCE_H_*/

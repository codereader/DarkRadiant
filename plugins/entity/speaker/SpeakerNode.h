#ifndef SPEAKERNODE_H_
#define SPEAKERNODE_H_

#include "nameable.h"
#include "editable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "transformlib.h"
#include "irenderable.h"
#include "selectionlib.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

#include "Speaker.h"

namespace entity {

class SpeakerNode :
	public EntityNode,
	public SelectableNode,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public TransformNode,
	public SelectionTestable,
	public Renderable,
	public Cullable,
	public Bounded,
	public TransformModifier,
	public TargetableNode
{
	friend class Speaker;

	Speaker m_contained;

public:
	SpeakerNode(IEntityClassPtr eclass);
	SpeakerNode(const SpeakerNode& other);

	virtual ~SpeakerNode();

	// Snappable implementation
	virtual void snapto(float snap);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// EntityNode implementation
	virtual Entity& getEntity();
	virtual void refreshModel();

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(
	    const VolumeTest& test, const Matrix4& localToWorld) const;

	// Namespaced implementation
	//virtual void setNamespace(INamespace& space);

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test);

	scene::INodePtr clone() const;

	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	// Nameable implementation
	virtual std::string name() const;
	
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);

	// Renderable implementation
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	void evaluateTransform();
	typedef MemberCaller<SpeakerNode, &SpeakerNode::evaluateTransform> EvaluateTransformCaller;
	
	void applyTransform();
	typedef MemberCaller<SpeakerNode, &SpeakerNode::applyTransform> ApplyTransformCaller;
};

} // namespace entity

#endif /*SPEAKERNODE_H_*/

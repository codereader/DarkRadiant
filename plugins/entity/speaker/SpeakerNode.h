#ifndef SPEAKERNODE_H_
#define SPEAKERNODE_H_

#include "nameable.h"
#include "editable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "instancelib.h"
#include "transformlib.h"

#include "Speaker.h"

namespace entity {

class SpeakerNode :
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public TransformNode,
	public EntityNode,
	public Namespaced
{
	InstanceSet m_instances;

	Speaker m_contained;

public:
	SpeakerNode(IEntityClassPtr eclass);
	SpeakerNode(const SpeakerNode& other);

	// Snappable implementation
	virtual void snapto(float snap);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// EntityNode implementation
	virtual Entity& getEntity();

	// Namespaced implementation
	virtual void setNamespace(INamespace& space);

	scene::INodePtr clone() const;

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(const scene::Path& path);

	// Nameable implementation
	virtual std::string name() const;
	
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);
};

} // namespace entity

#endif /*SPEAKERNODE_H_*/

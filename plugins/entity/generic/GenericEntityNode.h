#ifndef GENERICENTITYNODE_H_
#define GENERICENTITYNODE_H_

#include "nameable.h"
#include "editable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "instancelib.h"
#include "transformlib.h"

#include "GenericEntity.h"

namespace entity {

class GenericEntityNode :
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

	GenericEntity m_contained;

public:
	GenericEntityNode(IEntityClassPtr eclass);
	GenericEntityNode(const GenericEntityNode& other);

	// Snappable implementation
	virtual void snapto(float snap);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// EntityNode implementation
	virtual Entity& getEntity();

	// Namespaced implementation
	virtual void setNamespace(Namespace& space);

	scene::Node& clone() const;

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);

	// Nameable implementation
	virtual std::string name() const;
	
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);
};

} // namespace entity

#endif /*GENERICENTITYNODE_H_*/

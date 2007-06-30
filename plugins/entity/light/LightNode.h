#ifndef LIGHTNODE_H_
#define LIGHTNODE_H_

#include "scenelib.h"
#include "instancelib.h"

#include "Light.h"
#include "LightInstance.h"

class LightNode :
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public Editable,
	public TransformNode,
	public EntityNode,
	public Namespaced
{
	InstanceSet m_instances;
	Light m_contained;
	
public:
	LightNode(IEntityClassPtr eclass);
	LightNode(const LightNode& other);
	
	// EntityNode implementation
	virtual Entity& getEntity();
	
	// Namespaced implementation
	virtual void setNamespace(INamespace& space);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// Editable implementation
	virtual const Matrix4& getLocalPivot() const;

	// Snappable implementation
	virtual void snapto(float snap);

	scene::INodePtr clone() const;

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(const scene::Path& path);
	
	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);
}; // class LightNode

#endif /*LIGHTNODE_H_*/

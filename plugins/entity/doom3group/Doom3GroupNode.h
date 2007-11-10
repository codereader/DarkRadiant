#ifndef DOOM3GROUPNODE_H_
#define DOOM3GROUPNODE_H_

#include "igroupnode.h"
#include "Doom3Group.h"
#include "nameable.h"
#include "instancelib.h"
#include "scenelib.h"
#include "../namedentity.h"

namespace entity {

class Doom3GroupNode :
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public scene::GroupNode,
	public Nameable,
	public Snappable,
	public TransformNode,
	public TraversableNodeSet, // implements scene::Traversable
	public EntityNode,
	public Namespaced
{
	// The Child instances of this node
	InstanceSet m_instances;
	
	// The contained Doom3Group class
	Doom3Group m_contained;

public:
	Doom3GroupNode(IEntityClassPtr eclass);
	Doom3GroupNode(const Doom3GroupNode& other);

	~Doom3GroupNode();
	
	// EntityNode implementation
	virtual Entity& getEntity();

	// Namespaced implementation
	virtual void setNamespace(INamespace& space);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);

	// Snappable implementation
	virtual void snapto(float snap);

	scene::INodePtr clone() const;

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	
	void insert(const scene::Path& path, scene::Instance* instance);
	
	scene::Instance* erase(const scene::Path& path);

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated. 
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();
}; // class Doom3GroupNode

} // namespace entity

#endif /*DOOM3GROUPNODE_H_*/

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
	public scene::Traversable::Observer,
	public scene::GroupNode,
	public Nameable,
	public Snappable,
	public TransformNode,
	public scene::Traversable,
	public EntityNode,
	public Namespaced,
	public ModelSkin
{
	// The Child instances of this node
	InstanceSet m_instances;
	
	// The contained Doom3Group class
	Doom3Group m_contained;

public:
	Doom3GroupNode(IEntityClassPtr eclass);
	Doom3GroupNode(const Doom3GroupNode& other);
	
	~Doom3GroupNode();

	// ModelSkin implementation
	virtual void attach(ModuleObserver& observer);	
	virtual void detach(ModuleObserver& observer);
	virtual std::string getRemap(const std::string& name) const;

	// EntityNode implementation
	virtual Entity& getEntity();

	// Namespaced implementation
	virtual void setNamespace(INamespace& space);

	// scene::Traversable Implementation
	virtual void insert(scene::INodePtr node);
    virtual void erase(scene::INodePtr node);
    virtual void traverse(const Walker& walker);
    virtual bool empty() const;

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);

	// Snappable implementation
	virtual void snapto(float snap);

	scene::INodePtr clone() const;

	// scene::Traversable::Observer implementation
	void insertChild(scene::INodePtr child);
	void eraseChild(scene::INodePtr child);

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated. 
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

private:
	void construct();
	void destroy();

}; // class Doom3GroupNode

} // namespace entity

#endif /*DOOM3GROUPNODE_H_*/

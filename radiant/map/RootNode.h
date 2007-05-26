#ifndef MAPROOTNODE_H_
#define MAPROOTNODE_H_

#include "nameable.h"
#include "traverselib.h"
#include "selectionlib.h"
#include "UndoFileChangeTracker.h"
#include "scenelib.h"
#include "instancelib.h"

namespace map {

/** greebo: This is the root node of the map, it gets inserted as
 * 			the top node into the scenegraph. Each entity node is 
 * 			inserted as child node to this.
 * 
 * Note:	Inserting a child node to this MapRoot automatically
 * 			triggers an instantiation of this child node.
 * 
 * 			The contained InstanceSet functions as Traversable::Observer
 * 			and instantiates the node as soon as it gets notified about it.
 */
class RootNode : 
	public scene::Node, 
	public scene::Instantiable, 
	public Nameable,
	public TransformNode,
	public MapFile,
	public scene::Traversable
{
	IdentityTransform m_transform;
	TraversableNodeSet m_traverse;
	InstanceSet m_instances;
	
	// The actual name of the map
	std::string _name;
	
	UndoFileChangeTracker m_changeTracker;
public:
	// Constructor, pass the name of the map to it
	RootNode(const std::string& name);
	
	virtual ~RootNode();

  	// scene::Traversable Implementation
	virtual void insert(scene::INodePtr node);
    virtual void erase(scene::INodePtr node);
    virtual void traverse(const Walker& walker);
    virtual bool empty() const;
	
	// TransformNode implementation
	virtual const Matrix4& localToParent() const;
  
	// MapFile implementation
	virtual void save();
	virtual bool saved() const;
	virtual void changed();
	virtual void setChangedCallback(const Callback& changed);
	virtual std::size_t changes() const;

	// Nameable implementation
	std::string name() const;
  
	InstanceCounter m_instanceCounter;
	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	// Cloneable implementation
	scene::INodePtr clone() const;

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);
};

} // namespace map

inline scene::INodePtr NewMapRoot(const std::string& name) {
	return scene::INodePtr(new map::RootNode(name));
}

#endif /*MAPROOTNODE_H_*/

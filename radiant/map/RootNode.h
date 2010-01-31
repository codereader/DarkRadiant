#ifndef MAPROOTNODE_H_
#define MAPROOTNODE_H_

#include "nameable.h"
#include "inamespace.h"
#include "imap.h"
#include "selectionlib.h"
#include "UndoFileChangeTracker.h"
#include "scenelib.h"
#include "transformlib.h"

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
	public IMapRootNode,
	public Nameable,
	public TransformNode,
	public MapFile
{
	IdentityTransform m_transform;

	// The actual name of the map
	std::string _name;
	
	UndoFileChangeTracker m_changeTracker;

	// The namespace this node belongs to
	INamespacePtr _namespace;

public:
	// Constructor, pass the name of the map to it
	RootNode(const std::string& name);

	virtual ~RootNode();

	// Returns the reference to the Namespace of this rootnode
	INamespacePtr getNamespace();
	
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
	
	void setName(const std::string& name);
  
	InstanceCounter m_instanceCounter;
	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	// Override scene::Node methods
	virtual void onChildAdded(const scene::INodePtr& child);
	virtual void onChildRemoved(const scene::INodePtr& child);

	// Cloneable implementation
	scene::INodePtr clone() const;

	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

};
typedef boost::shared_ptr<RootNode> RootNodePtr;

} // namespace map

inline scene::INodePtr NewMapRoot(const std::string& name) {
	return scene::INodePtr(new map::RootNode(name));
}

#endif /*MAPROOTNODE_H_*/

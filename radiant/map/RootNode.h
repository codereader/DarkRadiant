#ifndef MAPROOTNODE_H_
#define MAPROOTNODE_H_

#include "nameable.h"
#include "inamespace.h"
#include "imap.h"
#include "selectionlib.h"
#include "UndoFileChangeTracker.h"
#include "scenelib.h"

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
  
	InstanceCounter m_instanceCounter;
	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	// Override the methods inherited from scene::Node 
	virtual void addChildNode(const scene::INodePtr& node);
	virtual void removeChildNode(const scene::INodePtr& node);

	// Cloneable implementation
	scene::INodePtr clone() const;

	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

};
typedef boost::shared_ptr<RootNode> RootNodePtr;

} // namespace map

inline scene::INodePtr NewMapRoot(const std::string& name) {
	scene::INodePtr root(new map::RootNode(name));
	root->setSelf(root);
	return root;
}

#endif /*MAPROOTNODE_H_*/

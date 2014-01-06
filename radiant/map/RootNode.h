#pragma once

#include "inamespace.h"
#include "imap.h"
#include "scene/Node.h"
#include "UndoFileChangeTracker.h"
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
	public IdentityTransform,
	public MapFile
{
private:
	// The actual name of the map
	std::string _name;

	UndoFileChangeTracker _changeTracker;

	// The namespace this node belongs to
	INamespacePtr _namespace;

	AABB _emptyAABB;

	std::size_t _instanceCounter;

public:
	// Constructor, pass the name of the map to it
	RootNode(const std::string& name);

	virtual ~RootNode();

	// Returns the reference to the Namespace of this rootnode
	INamespacePtr getNamespace();

	// MapFile implementation
	virtual void save();
	virtual bool saved() const;
	virtual void changed();
	virtual void setChangedCallback(const boost::function<void()>& changed);
	virtual std::size_t changes() const;

	// Renderable implementation (empty)
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
	{}

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
	{}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	std::string name() const;
	Type getNodeType() const;

	void setName(const std::string& name);

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	// Override scene::Node methods
	virtual void onChildAdded(const scene::INodePtr& child);
	virtual void onChildRemoved(const scene::INodePtr& child);

	// Cloneable implementation
	scene::INodePtr clone() const;

	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	const AABB& localAABB() const
	{
		return _emptyAABB;
	}
};
typedef boost::shared_ptr<RootNode> RootNodePtr;

} // namespace map

inline scene::INodePtr NewMapRoot(const std::string& name) {
	return scene::INodePtr(new map::RootNode(name));
}

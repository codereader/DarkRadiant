#pragma once

#include "inamespace.h"
#include "imap.h"
#include "scene/Node.h"
#include "UndoFileChangeTracker.h"
#include "transformlib.h"

namespace map 
{

/** greebo: This is the root node of the map, it gets inserted as
 * 			the top node into the scenegraph. Each entity node is
 * 			inserted as child node to this.
 *
 * Note:	Inserting a child node to this MapRoot automatically
 * 			triggers an instantiation of this child node.
 */
class RootNode :
	public scene::Node,
    public scene::IMapRootNode,
	public IdentityTransform,
    public IMapFileChangeTracker
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
    const INamespacePtr& getNamespace() override;
    IMapFileChangeTracker& getUndoChangeTracker() override;

	// MapFile implementation
	virtual void save();
	virtual bool saved() const;
	virtual void changed();
	virtual void setChangedCallback(const std::function<void()>& changed);
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

	void onInsertIntoScene(IMapFileChangeTracker* map);
	void onRemoveFromScene(IMapFileChangeTracker* map);

	// Override scene::Node methods
	virtual void onChildAdded(const scene::INodePtr& child);
	virtual void onChildRemoved(const scene::INodePtr& child);

    virtual void onInsertIntoScene(IMapRootNode& root) override;
    virtual void onRemoveFromScene(IMapRootNode& root) override;

	const AABB& localAABB() const
	{
		return _emptyAABB;
	}
};
typedef std::shared_ptr<RootNode> RootNodePtr;

} // namespace map

#pragma once

#include "inamespace.h"
#include "imap.h"
#include "ientity.h"
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
    protected UndoFileChangeTracker
{
private:
	// The actual name of the map
	std::string _name;

	// The namespace this node belongs to
	INamespacePtr _namespace;

    // The target tracker
    ITargetManagerPtr _targetManager;

	AABB _emptyAABB;

public:
	// Constructor, pass the name of the map to it
	RootNode(const std::string& name);

	virtual ~RootNode();

	// Returns the reference to the Namespace of this rootnode
    const INamespacePtr& getNamespace() override;
    IMapFileChangeTracker& getUndoChangeTracker() override;
    ITargetManager& getTargetManager() override;

	// Renderable implementation (empty)
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override
	{}

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override
	{}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

	std::string name() const override;
	Type getNodeType() const override;

	void setName(const std::string& name);

	// Override scene::Node methods
	virtual void onChildAdded(const scene::INodePtr& child) override;
	virtual void onChildRemoved(const scene::INodePtr& child) override;

    virtual void onInsertIntoScene(IMapRootNode& root) override;
    virtual void onRemoveFromScene(IMapRootNode& root) override;

	const AABB& localAABB() const override
	{
		return _emptyAABB;
	}
};
typedef std::shared_ptr<RootNode> RootNodePtr;

} // namespace map

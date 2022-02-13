#pragma once

#include "inamespace.h"
#include "imap.h"
#include "ilayer.h"
#include "ientity.h"
#include "iselectiongroup.h"
#include "iselectionset.h"
#include "scene/Node.h"
#include "UndoFileChangeTracker.h"
#include "transformlib.h"
#include "KeyValueStore.h"
#include "undo/UndoSystem.h"
#include <sigc++/connection.h>

namespace map 
{

/** 
 * greebo: This is the root node of the map, it gets inserted as
 * the top node into the scenegraph. Each entity node is
 * inserted as child node to this.
 *
 * Note: Inserting a child node to this MapRoot automatically
 * triggers an "instantiation" of this child node, which can be
 * seen as "activation" of this node. In contrast to nodes on the 
 * undo stack which are "not instantiated"/inactive.
 */
class RootNode :
	public scene::Node,
    public scene::IMapRootNode,
	public IdentityTransform,
    public UndoFileChangeTracker,
	public KeyValueStore
{
private:
	// The actual name of the map
	std::string _name;

	// The namespace this node belongs to
	INamespacePtr _namespace;

    ITargetManagerPtr _targetManager;

    selection::ISelectionGroupManager::Ptr _selectionGroupManager;

    selection::ISelectionSetManager::Ptr _selectionSetManager;

    scene::ILayerManager::Ptr _layerManager;

    IUndoSystem::Ptr _undoSystem;

	AABB _emptyAABB;

    sigc::connection _undoEventHandler;

public:
	// Constructor, pass the name of the map to it
	RootNode(const std::string& name);

	virtual ~RootNode();

	void setName(const std::string& name) override;
	// Returns the reference to the Namespace of this rootnode
    const INamespacePtr& getNamespace() override;
    IMapFileChangeTracker& getUndoChangeTracker() override;
    ITargetManager& getTargetManager() override;
    selection::ISelectionGroupManager& getSelectionGroupManager() override;
    selection::ISelectionSetManager& getSelectionSetManager() override;
    scene::ILayerManager& getLayerManager() override;
    IUndoSystem& getUndoSystem() override;

	// Renderable implementation (empty)
    void onPreRender(const VolumeTest& volume) override
    {}
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override
    {}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

	std::string name() const override;
	Type getNodeType() const override;

	// Override scene::Node methods
	virtual void onChildAdded(const scene::INodePtr& child) override;
	virtual void onChildRemoved(const scene::INodePtr& child) override;

	const AABB& localAABB() const override
	{
		return _emptyAABB;
	}

    RenderSystemPtr getRenderSystem() const override
    {
        return Node::getRenderSystem();
    }
};
typedef std::shared_ptr<RootNode> RootNodePtr;

} // namespace map

#pragma once

#include "imap.h"
#include "mapfile.h"
#include "ilayer.h"
#include "ientity.h"
#include "iselectiongroup.h"
#include "Node.h"
#include "inamespace.h"
#include "UndoFileChangeTracker.h"
#include "KeyValueStore.h"

namespace scene
{

// A very simple implementation of a Map Root Node
// for use in the preview widget's scenes.
class BasicRootNode :
    public IMapRootNode,
    public Node,
    public KeyValueStore
{
private:
    INamespacePtr _namespace;
    UndoFileChangeTracker _changeTracker;
    ITargetManagerPtr _targetManager;
    selection::ISelectionGroupManager::Ptr _selectionGroupManager;
    ILayerManager::Ptr _layerManager;
    AABB _emptyAABB;

public:
    BasicRootNode()
    {
        _namespace = GlobalNamespaceFactory().createNamespace();
        _targetManager = GlobalEntityCreator().createTargetManager();
        _selectionGroupManager = GlobalSelectionGroupModule().createSelectionGroupManager();
        _layerManager = GlobalLayerModule().createLayerManager();
    }

    virtual ~BasicRootNode()
    {}

    const INamespacePtr& getNamespace() override
    {
        return _namespace;
    }

    IMapFileChangeTracker& getUndoChangeTracker() override
    {
        return _changeTracker;
    }

    ITargetManager& getTargetManager() override
    {
        return *_targetManager;
    }

    selection::ISelectionGroupManager& getSelectionGroupManager() override
    {
        return *_selectionGroupManager;
    }

    scene::ILayerManager& getLayerManager() override
    {
        return *_layerManager;
    }

    const AABB& localAABB() const override
    {
        return _emptyAABB;
    }

    Type getNodeType() const override
    {
        return Type::MapRoot;
    }

    // Renderable implementation (empty)
    void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override
    {}

    void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override
    {}

	std::size_t getHighlightFlags() override
    {
        return Highlight::NoHighlight; // never highlighted
    }
};

}

#pragma once

#include "imap.h"
#include "ilayer.h"
#include "ientity.h"
#include "iundo.h"
#include "iselectiongroup.h"
#include "iselectionset.h"
#include "Node.h"
#include "inamespace.h"
#include "UndoFileChangeTracker.h"
#include "KeyValueStore.h"

namespace scene
{

// A very simple implementation of a Map Root Node
// for use in the preview widget's scenes.
class BasicRootNode final :
    public IMapRootNode,
    public Node,
    public KeyValueStore
{
private:
    std::string _name;
    INamespacePtr _namespace;
    UndoFileChangeTracker _changeTracker;
    ITargetManagerPtr _targetManager;
    selection::ISelectionGroupManager::Ptr _selectionGroupManager;
    selection::ISelectionSetManager::Ptr _selectionSetManager;
    ILayerManager::Ptr _layerManager;
    IUndoSystem::Ptr _undoSystem;
    AABB _emptyAABB;

public:
    BasicRootNode()
    {
        _namespace = GlobalNamespaceFactory().createNamespace();
        _targetManager = GlobalEntityModule().createTargetManager();
        _selectionGroupManager = GlobalSelectionGroupModule().createSelectionGroupManager();
        _selectionSetManager = GlobalSelectionSetModule().createSelectionSetManager();
        _layerManager = GlobalLayerModule().createLayerManager();
        _undoSystem = GlobalUndoSystemFactory().createUndoSystem();
    }

    void setName(const std::string& name) override
    {
        _name = name;
    }

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

    selection::ISelectionSetManager& getSelectionSetManager() override
    {
        return *_selectionSetManager;
    }

    scene::ILayerManager& getLayerManager() override
    {
        return *_layerManager;
    }

    IUndoSystem& getUndoSystem() override
    {
        return *_undoSystem;
    }

    const AABB& localAABB() const override
    {
        return _emptyAABB;
    }

    Type getNodeType() const override
    {
        return Type::MapRoot;
    }

    void onPreRender(const VolumeTest& volume) override
    {}

    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override
    {}

	std::size_t getHighlightFlags() override
    {
        return Highlight::NoHighlight; // never highlighted
    }

    RenderSystemPtr getRenderSystem() const override
    {
        return Node::getRenderSystem();
    }
};

}

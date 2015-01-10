#pragma once

#include "imap.h"
#include "mapfile.h"
#include "Node.h"
#include "inamespace.h"
#include "UndoFileChangeTracker.h"

namespace scene
{

// A very simple implementation of a Map Root Node
// for use in the preview widget's scenes.
class BasicRootNode :
    public IMapRootNode,
    public Node
{
private:
    INamespacePtr _namespace;
    UndoFileChangeTracker _changeTracker;
    AABB _emptyAABB;

public:
    BasicRootNode()
    {
        _namespace = GlobalNamespaceFactory().createNamespace();
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

    bool isHighlighted() const override
    {
        return false; // never highlighted
    }
};

}

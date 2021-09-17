#pragma once

#include <list>
#include <sigc++/connection.h>
#include "itexturetoolmodel.h"

namespace textool
{

class TextureToolSceneGraph :
    public ITextureToolSceneGraph
{
private:
    sigc::connection _sceneSelectionChanged;

    bool _selectionNeedsRescan;

    std::list<INode::Ptr> _nodes;

public:
    TextureToolSceneGraph();

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    void foreachNode(const std::function<bool(const INode::Ptr&)>& functor) override;

private:
    void onSceneSelectionChanged(const ISelectable& selectable);
    void ensureSceneIsAnalysed();
};

}

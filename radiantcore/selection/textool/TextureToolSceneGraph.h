#pragma once

#include <list>
#include <sigc++/connection.h>
#include "itexturetoolmodel.h"
#include "messages/TextureChanged.h"

namespace textool
{

class TextureToolSceneGraph :
    public ITextureToolSceneGraph
{
private:
    sigc::connection _sceneSelectionChanged;
    std::size_t _textureChangedHandler;

    bool _selectionNeedsRescan;
    bool _activeMaterialNeedsRescan;

    std::list<INode::Ptr> _nodes;

    // The single active material. Is empty if the scene graph has no items
    std::string _activeMaterial;

public:
    TextureToolSceneGraph();

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    void foreachNode(const std::function<bool(const INode::Ptr&)>& functor) override;

    const std::string& getActiveMaterial() override;

private:
    void onSceneSelectionChanged(const ISelectable& selectable);
    void onTextureChanged(radiant::TextureChangedMessage& msg);

    void ensureSceneIsAnalysed();
};

}

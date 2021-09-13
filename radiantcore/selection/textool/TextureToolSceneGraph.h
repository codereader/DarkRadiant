#pragma once

#include <sigc++/connection.h>
#include "itexturetoolmodel.h"

namespace textool
{

class TextureToolSceneGraph :
    public ITextureToolSceneGraph
{
private:
    sigc::connection _sceneSelectionChanged;

public:
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    void foreachNode(const std::function<bool(const INode::Ptr&)>& functor) override;

private:
    void onSceneSelectionChanged(const ISelectable& selectable);
};

}

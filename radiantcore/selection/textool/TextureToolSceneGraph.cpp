#include "TextureToolSceneGraph.h"

#include "iselection.h"
#include "module/StaticModule.h"

namespace textool
{

const std::string& TextureToolSceneGraph::getName() const
{
    static std::string _name(MODULE_TEXTOOL_SCENEGRAPH);
    return _name;
}

const StringSet& TextureToolSceneGraph::getDependencies() const
{
    static StringSet _dependencies{ MODULE_SELECTIONSYSTEM };
    return _dependencies;
}

void TextureToolSceneGraph::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    _sceneSelectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
        sigc::mem_fun(this, &TextureToolSceneGraph::onSceneSelectionChanged)
    );
}

void TextureToolSceneGraph::shutdownModule()
{
    _sceneSelectionChanged.disconnect();
}

void TextureToolSceneGraph::foreachNode(const std::function<bool(const INode::Ptr&)>& functor)
{

}

void TextureToolSceneGraph::onSceneSelectionChanged(const ISelectable& selectable)
{
    // Mark our own selection as dirty
}

module::StaticModule<TextureToolSceneGraph> _textureToolSceneGraphModule;

}

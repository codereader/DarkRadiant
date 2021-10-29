#include "TextureToolSceneGraph.h"

#include "iselection.h"
#include "ibrush.h"
#include "ipatch.h"
#include "module/StaticModule.h"
#include "selectionlib.h"

#include "FaceNode.h"
#include "PatchNode.h"

namespace textool
{

TextureToolSceneGraph::TextureToolSceneGraph() :
    _selectionNeedsRescan(true)
{}

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

    _textureChangedHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::TextureChanged,
        radiant::TypeListener<radiant::TextureChangedMessage>(
            sigc::mem_fun(this, &TextureToolSceneGraph::onTextureChanged)));
}

void TextureToolSceneGraph::shutdownModule()
{
    GlobalRadiantCore().getMessageBus().removeListener(_textureChangedHandler);
    _selectionNeedsRescan = false;
    _nodes.clear();
    _sceneSelectionChanged.disconnect();
}

void TextureToolSceneGraph::foreachNode(const std::function<bool(const INode::Ptr&)>& functor)
{
    ensureSceneIsAnalysed();

    for (const auto& node : _nodes)
    {
        if (!functor(node))
        {
            break;
        }
    }
}

const std::string& TextureToolSceneGraph::getActiveMaterial()
{
    ensureSceneIsAnalysed();
    ensureActiveMaterialIsAnalysed();

    return _activeMaterial;
}

void TextureToolSceneGraph::ensureActiveMaterialIsAnalysed()
{
    if (!_activeMaterialNeedsRescan) return;

    _activeMaterialNeedsRescan = false;
    _activeMaterial = selection::getShaderFromSelection();
}

void TextureToolSceneGraph::ensureSceneIsAnalysed()
{
    if (!_selectionNeedsRescan) return;
    
    _selectionNeedsRescan = false;
    _activeMaterialNeedsRescan = false;

    _nodes.clear();

    _activeMaterial = selection::getShaderFromSelection();
    if (_activeMaterial.empty()) return;

    if (GlobalSelectionSystem().countSelectedComponents() > 0)
    {
        // Check each selected face
        GlobalSelectionSystem().foreachFace([&](IFace& face)
        {
            _nodes.emplace_back(std::make_shared<FaceNode>(face));
        });
    }
    
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        if (Node_isBrush(node))
        {
            auto brush = Node_getIBrush(node);
            assert(brush);

            for (auto i = 0; i < brush->getNumFaces(); ++i)
            {
                _nodes.emplace_back(std::make_shared<FaceNode>(brush->getFace(i)));
            }
        }
        else if (Node_isPatch(node))
        {
            _nodes.emplace_back(std::make_shared<PatchNode>(*Node_getIPatch(node)));
        }
    });
}

void TextureToolSceneGraph::onSceneSelectionChanged(const ISelectable& selectable)
{
    // Mark our own selection as dirty
    _selectionNeedsRescan = true;
}

void TextureToolSceneGraph::onTextureChanged(radiant::TextureChangedMessage& msg)
{
    _activeMaterialNeedsRescan = true;
}

module::StaticModule<TextureToolSceneGraph> _textureToolSceneGraphModule;

}

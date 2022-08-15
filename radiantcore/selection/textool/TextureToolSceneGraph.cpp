#include "TextureToolSceneGraph.h"

#include "iselection.h"
#include "ibrush.h"
#include "ipatch.h"
#include "module/StaticModule.h"
#include "selectionlib.h"

#include "selection/algorithm/Primitives.h"
#include "FaceNode.h"
#include "PatchNode.h"

namespace textool
{

TextureToolSceneGraph::TextureToolSceneGraph() :
    _selectionNeedsRescan(true),
    _activeMaterialNeedsRescan(true)
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
    _selectionNeedsRescan = false;
    _activeMaterialNeedsRescan = false;
    clearFaceObservers();
    _nodes.clear();
    _sceneSelectionChanged.disconnect();
    GlobalRadiantCore().getMessageBus().removeListener(_textureChangedHandler);
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

    return _activeMaterial;
}

void TextureToolSceneGraph::ensureSceneIsAnalysed()
{
    // First, check if the material has changed
    if (_activeMaterialNeedsRescan)
    {
        _activeMaterialNeedsRescan = false;

        auto material = selection::getShaderFromSelection();

        if (material != _activeMaterial)
        {
            _activeMaterial = std::move(material);

            // The material changed, we need to rebuild the nodes
            _selectionNeedsRescan = true;
        }
    }

    if (!_selectionNeedsRescan) return;

    _selectionNeedsRescan = false;
    clearFaceObservers();
    _nodes.clear();

    // No unique material, leave everything empty
    if (_activeMaterial.empty()) return;

    if (GlobalSelectionSystem().countSelectedComponents() > 0)
    {
        selection::algorithm::forEachSelectedFaceComponent([&](IFace& face)
        {
            createFaceNode(face);
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
                createFaceNode(brush->getFace(i));
            }
        }
        else if (Node_isPatch(node))
        {
            _nodes.emplace_back(std::make_shared<PatchNode>(*Node_getIPatch(node)));
        }
    });
}

void TextureToolSceneGraph::clearFaceObservers()
{
    for (auto& conn : _faceObservers)
    {
        conn.disconnect();
    }

    _faceObservers.clear();
}

void TextureToolSceneGraph::createFaceNode(IFace& face)
{
    _nodes.emplace_back(std::make_shared<FaceNode>(face));

    // When faces are destroyed, the selection is out of date, queue a rescan
    _faceObservers.emplace_back(face.signal_faceDestroyed().connect(
        [this]() { _selectionNeedsRescan = true; }
    ));
}

void TextureToolSceneGraph::onSceneSelectionChanged(const ISelectable& selectable)
{
    // Mark our own selection as dirty
    _activeMaterialNeedsRescan = true;
    _selectionNeedsRescan = true;
}

void TextureToolSceneGraph::onTextureChanged(radiant::TextureChangedMessage& msg)
{
    _activeMaterialNeedsRescan = true;
}

module::StaticModuleRegistration<TextureToolSceneGraph> _textureToolSceneGraphModule;

}

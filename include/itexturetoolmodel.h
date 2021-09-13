#pragma once

#include "imodule.h"
#include "inode.h"

namespace textool
{

// The base element of every node in the ITextureToolSceneGraph
class INode
{
public:
    virtual ~INode() {}

    using Ptr = std::shared_ptr<INode>;

    // The scene node this texture tool node is relating to
    virtual scene::INodePtr getSceneNode() = 0;
};

/**
 * The scene graph of all texture tool items. From all the selected
 * items in the regular SceneGraph the texture-editable elements
 * are extracted and put into this graph.
 * 
 * It can have multiple top-level elements (faces, patches) each of which 
 * can have an arbitrary number of child elements (vertices).
 * 
 * Like in the regular scene graph, some of the elements have special 
 * properties like being renderable or transformable, which manifests
 * in them implementing the corresponding interface.
 */
class ITextureToolSceneGraph :
    public RegisterableModule
{
public:
    virtual ~ITextureToolSceneGraph() {}

    // Iterate over every node in this graph calling the given functor
    virtual void foreachNode(const std::function<bool(const INode::Ptr&)>& functor) = 0;
};

}

constexpr const char* const MODULE_TEXTOOL_SCENEGRAPH("TextureToolSceneGraph");

inline textool::ITextureToolSceneGraph& GlobalTextureToolSceneGraph()
{
    static module::InstanceReference<textool::ITextureToolSceneGraph> _reference(MODULE_TEXTOOL_SCENEGRAPH);
    return _reference;
}

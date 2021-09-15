#pragma once

#include "imodule.h"
#include "inode.h"
#include "Bounded.h"
#include "iselection.h"
#include "iselectiontest.h"

class Matrix3;

namespace textool
{

/**
 * A transformable node in the texture tool scene. This is usually
 * implemented by Faces and Patches. They will accept a given transformation
 * and apply it to their selected child elements.
 * For faces, this could mean that three selected vertices (out of four)
 * will be transformed by the given matrix. It's possible that unselected
 * winding vertices will be affected by this transformation, due to the nature
 * of the texture projection. 
 * Patch vertices can be transformed independently.
 */
class ITransformable
{
public:
    /**
     * Applies the given transform to all selected components of this node.
     */
    virtual void applyTransformToSelected(const Matrix3& transform) = 0;
};

// The base element of every node in the ITextureToolSceneGraph
class INode :
    public ITransformable,
    public Bounded,
    public ISelectable,
    public SelectionTestable
{
public:
    virtual ~INode() {}

    using Ptr = std::shared_ptr<INode>;
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
    // Collection should not be modified during iteration
    virtual void foreachNode(const std::function<bool(const INode::Ptr&)>& functor) = 0;

    // Iterate over every selected node in this graph calling the given functor
    // Collection should not be modified during iteration
    virtual void foreachSelectedNode(const std::function<bool(const INode::Ptr&)>& functor) = 0;
};

}

constexpr const char* const MODULE_TEXTOOL_SCENEGRAPH("TextureToolSceneGraph");

inline textool::ITextureToolSceneGraph& GlobalTextureToolSceneGraph()
{
    static module::InstanceReference<textool::ITextureToolSceneGraph> _reference(MODULE_TEXTOOL_SCENEGRAPH);
    return _reference;
}

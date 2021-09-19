#pragma once

#include "imodule.h"
#include "inode.h"
#include "Bounded.h"
#include "iselection.h"
#include "iselectiontest.h"
#include "imanipulator.h"
#include <sigc++/signal.h>

class Matrix3;
class IFace;
class IPatch;

namespace textool
{

// Enumeration of possible texture tool selection modes
enum class SelectionMode
{
    Surface,
    Vertex,
};

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
    // Let this object save a snapshot of its current texture state to have something
    // to base the upcoming transformation on
    virtual void beginTransformation() = 0;

    // Move the state back to the base state we saved in beginTransformation()
    // Is called right before applyTransformationToSelected() is invoked with a new transform
    virtual void revertTransformation() = 0;

    // Applies the given transform to all selected components of this node.
    virtual void transform(const Matrix3& transform) = 0;

    // "Saves" the current transformed state as the new base state
    virtual void commitTransformation() = 0;
};

// A Texture Tool node that consists of one or more selectable components
class IComponentSelectable
{
public:
    using Ptr = std::shared_ptr<IComponentSelectable>;

    virtual ~IComponentSelectable() {}

    // True if this node has at least one selected component (e.g. a vertex)
    virtual bool hasSelectedComponents() const = 0;

    // Unselect all components of this node
    virtual void clearComponentSelection() = 0;

    // Perform a selection test using the given selector and test volume
    virtual void testSelectComponents(Selector& selector, SelectionTest& test) = 0;

    // Returns the bounds containing all the selected vertices
    virtual AABB getSelectedComponentBounds() = 0;
};

// A Texture Tool node that allows its components to be transformed
class IComponentTransformable
{
public:
    virtual ~IComponentTransformable() {}

    // Apply the given transformation matrix to all selected components
    virtual void transformComponents(const Matrix3& transform) = 0;
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

    // Renders this node, with all coords relative to UV space origin
    virtual void render(SelectionMode mode) = 0;
};

// Node representing a single brush face
class IFaceNode :
    public virtual INode,
    public virtual IComponentSelectable,
    public virtual IComponentTransformable
{
public:
    virtual ~IFaceNode() {}

    using Ptr = std::shared_ptr<IFaceNode>;

    virtual IFace& getFace() = 0;
};

// Node representing a patch
class IPatchNode :
    public virtual INode,
    public virtual IComponentSelectable,
    public virtual IComponentTransformable
{
public:
    virtual ~IPatchNode() {}

    using Ptr = std::shared_ptr<IPatchNode>;

    virtual IPatch& getPatch() = 0;
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
};

class ITextureToolSelectionSystem :
    public RegisterableModule
{
public:
    // Iterate over every selected node in this graph calling the given functor
    // Collection should not be modified during iteration
    virtual void foreachSelectedNode(const std::function<bool(const INode::Ptr&)>& functor) = 0;

    // Iterate over every node that has at least one component selected, visiting the given functor
    // Collection should not be modified during iteration
    virtual void foreachSelectedComponentNode(const std::function<bool(const INode::Ptr&)>& functor) = 0;

    virtual SelectionMode getMode() const = 0;
    virtual void setMode(SelectionMode mode) = 0;
    virtual sigc::signal<void, SelectionMode>& signal_selectionModeChanged() = 0;

    virtual void selectPoint(SelectionTest& test, SelectionSystem::EModifier modifier) = 0;
    virtual void selectArea(SelectionTest& test, SelectionSystem::EModifier modifier) = 0;

    // Returns the ID of the registered manipulator
    virtual std::size_t registerManipulator(const selection::ITextureToolManipulator::Ptr& manipulator) = 0;
    virtual void unregisterManipulator(const selection::ITextureToolManipulator::Ptr& manipulator) = 0;

    virtual selection::IManipulator::Type getActiveManipulatorType() = 0;

    // Returns the currently active Manipulator, which is always non-null
    virtual const selection::ITextureToolManipulator::Ptr& getActiveManipulator() = 0;
    virtual void setActiveManipulator(std::size_t manipulatorId) = 0;
    virtual void setActiveManipulator(selection::IManipulator::Type manipulatorType) = 0;

    virtual sigc::signal<void, selection::IManipulator::Type>& signal_activeManipulatorChanged() = 0;

    virtual Matrix4 getPivot2World() = 0;
    virtual void onManipulationStart() = 0;
    virtual void onManipulationChanged() = 0;
    virtual void onManipulationFinished() = 0;
    virtual void onManipulationCancelled() = 0;
};

}

constexpr const char* const MODULE_TEXTOOL_SCENEGRAPH("TextureToolSceneGraph");
constexpr const char* const MODULE_TEXTOOL_SELECTIONSYSTEM("TextureToolSelectionSystem");

inline textool::ITextureToolSceneGraph& GlobalTextureToolSceneGraph()
{
    static module::InstanceReference<textool::ITextureToolSceneGraph> _reference(MODULE_TEXTOOL_SCENEGRAPH);
    return _reference;
}

inline textool::ITextureToolSelectionSystem& GlobalTextureToolSelectionSystem()
{
    static module::InstanceReference<textool::ITextureToolSelectionSystem> _reference(MODULE_TEXTOOL_SELECTIONSYSTEM);
    return _reference;
}


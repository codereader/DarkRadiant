#pragma once

#include <cstddef>
#include "imodule.h"
#include <memory>
#include <sigc++/signal.h>
#include "imanipulator.h"

class RenderableCollector;

class ISelectable;
class SelectionTest;

namespace scene
{
	class INode;
	typedef std::shared_ptr<INode> INodePtr;
}

template<typename Element> class BasicVector2;
typedef BasicVector2<double> Vector2;
template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
template<typename Element> class BasicVector4;
typedef BasicVector4<double> Vector4;
class Matrix4;
class Quaternion;

typedef sigc::signal<void, const ISelectable&> SelectionChangedSignal;
typedef std::function<void(const ISelectable&)> SelectionChangedSlot;

class SelectionInfo;
class Face;
class IFace;
class Brush;
class IPatch;

namespace selection
{ 
    struct WorkZone; 

    // The possible modes when in "component manipulation mode"
    // Used by the SelectionSystem as well as the Texture Tool
    enum class ComponentSelectionMode
    {
        Default,
        Vertex,
        Edge,
        Face,
    };

    // The selection strategy determining the scene objects chosen for selection tests
    enum class SelectionMode
    {
        Entity,         // Entities only
        Primitive,      // Primitives (no components), entities take precedence
        GroupPart,      // Child Primitives of non-worldspawn entities
        Component,      // Components
        MergeAction,    // Merge Action nodes only
    };
}

namespace selection
{

class SelectionSystem :
	public RegisterableModule
{
public:
  enum EModifier {
	eManipulator,	// greebo: This is the standard case (drag, click without modifiers)
	eToggle,	// This is for Shift-Clicks to toggle the selection of an instance
	eReplace,	// This is active if the mouse is moved to a NEW location and Alt-Shift is held
	eCycle,		// This is active if the mouse STAYS at the same position and Alt-Shift is held
  };

	/** greebo: An SelectionSystem::Observer gets notified
	 * as soon as the selection is changed.
	 */
	class Observer
	{
	public:
		virtual ~Observer() {}
		/** greebo: This gets called upon selection change.
		 *
		 * @instance: The instance that got affected (this may also be the parent brush of a FaceInstance).
		 * @isComponent: is TRUE if the changed selectable is a component (like a FaceInstance, VertexInstance).
		 */
		virtual void selectionChanged(const scene::INodePtr& node, bool isComponent) = 0;
	};

	virtual void addObserver(Observer* observer) = 0;
	virtual void removeObserver(Observer* observer) = 0;

	// Returns the ID of the registered manipulator
	virtual std::size_t registerManipulator(const ISceneManipulator::Ptr& manipulator) = 0;
	virtual void unregisterManipulator(const ISceneManipulator::Ptr& manipulator) = 0;

	virtual IManipulator::Type getActiveManipulatorType() = 0;

	// Returns the currently active Manipulator, which is always non-null
	virtual const ISceneManipulator::Ptr& getActiveManipulator() = 0;
	virtual void setActiveManipulator(std::size_t manipulatorId) = 0;
	virtual void setActiveManipulator(IManipulator::Type manipulatorType) = 0;

	virtual sigc::signal<void, IManipulator::Type>& signal_activeManipulatorChanged() = 0;

	virtual const SelectionInfo& getSelectionInfo() = 0;

    virtual void setSelectionMode(SelectionMode mode) = 0;
    virtual SelectionMode getSelectionMode() const = 0;

    virtual void SetComponentMode(ComponentSelectionMode mode) = 0;
    virtual ComponentSelectionMode ComponentMode() const = 0;

	virtual sigc::signal<void, SelectionMode>& signal_selectionModeChanged() = 0;
	virtual sigc::signal<void, ComponentSelectionMode>& signal_componentModeChanged() = 0;

  virtual std::size_t countSelected() const = 0;
  virtual std::size_t countSelectedComponents() const = 0;
  virtual void onSelectedChanged(const scene::INodePtr& node, const ISelectable& selectable) = 0;
  virtual void onComponentSelection(const scene::INodePtr& node, const ISelectable& selectable) = 0;

	virtual scene::INodePtr ultimateSelected() = 0;
	virtual scene::INodePtr penultimateSelected() = 0;

    /**
     * \brief
     * Set the selection status of all objects in the scene.
     *
     * \param selected
     * true to select all objects, false to deselect all objects.
     */
    virtual void setSelectedAll(bool selected) = 0;

  virtual void setSelectedAllComponents(bool selected) = 0;

    /**
     * @brief Visitor interface for the selection system.
     *
     * This defines the Visitor interface which is used in the foreachSelected()
     * and foreachSelectedComponent() visit methods.
     */
	class Visitor
	{
	public:
		virtual ~Visitor() {}

        /// Called by the selection system for each visited node.
		virtual void visit(const scene::INodePtr& node) const = 0;
	};

    /// Use the provided Visitor object to enumerate each selected node.
    void foreachSelected(const Visitor& visitor)
    {
        foreachSelected([&visitor](const scene::INodePtr& p)
                        { visitor.visit(p); });
    }

    /**
     * @brief
     * Use the provided Visitor object to enumerate each selected component.
     */
    virtual void foreachSelectedComponent(const Visitor& visitor) = 0;

	/// Call the given functor to enumerate each selected node.
	virtual void foreachSelected(const std::function<void(const scene::INodePtr&)>& functor) = 0;

	/**
     * @brief
     * Use the provided functor to enumerate each selected component.
     */
	virtual void foreachSelectedComponent(const std::function<void(const scene::INodePtr&)>& functor) = 0;

	/**
	 * Call the given functor for each selected brush. Selected group nodes like func_statics
	 * are traversed recursively, invoking the functor for each visible brush in question.
	 */
	virtual void foreachBrush(const std::function<void(Brush&)>& functor) = 0;

	/**
	 * Call the given functor for each selected face. Selected group nodes like func_statics
	 * are traversed recursively, invoking the functor for each visible face in question.
	 * Singly selected faces (those which have been selected in component mode) are
	 * considered as well by this method.
	 */
	virtual void foreachFace(const std::function<void(IFace&)>& functor) = 0;

	/**
	 * Call the given functor for each selected patch. Selected group nodes like func_statics
	 * are traversed recursively, invoking the functor for each visible patch in question.
	 */
	virtual void foreachPatch(const std::function<void(IPatch&)>& functor) = 0;

	// Returns the number of currently selected faces
	virtual std::size_t getSelectedFaceCount() = 0;

	// Returns the reference to the singly selected face
	// Calling this will cause an cmd::ExecutionFailure if getSelectedFaceCount() != 1
	virtual IFace& getSingleSelectedFace() = 0;

    /// Signal emitted when the selection is changed
    virtual SelectionChangedSignal signal_selectionChanged() const = 0;

	virtual const Matrix4& getPivot2World() = 0;
    virtual void pivotChanged() = 0;

	// Feedback events invoked by the ManipulationMouseTool
	virtual void onManipulationStart() = 0;
	virtual void onManipulationChanged() = 0;
	virtual void onManipulationEnd() = 0;
	virtual void onManipulationCancelled() = 0;

	virtual void selectPoint(SelectionTest& test, EModifier modifier, bool face) = 0;
	virtual void selectArea(SelectionTest& test, EModifier modifier, bool face) = 0;

	/**
	 * Returns the current "work zone", which is defined by the
	 * currently selected elements. Each time a scene node is selected,
	 * the workzone is adjusted to surround the current selection.
	 * Deselecting nodes doesn't change the workzone.
	 *
	 * The result is used to determine the "third" component of operations
	 * performed in the 2D views, like placing an entity.
	 *
	 * Note: the struct is defined in selectionlib.h.
	 */
	virtual const WorkZone& getWorkZone() = 0;

	// Returns the center point of the current selection
	virtual Vector3 getCurrentSelectionCenter() = 0;

    // Toggles selection focus mode (only possible with a non-empty selection)
    // After activating, only items that are part of the set can be selected and manipulated.
    // Throws cmd::ExecutionNotPossible when trying to activate with an empty selection
    virtual void toggleSelectionFocus() = 0;

    // Returns true when focus mode is active
    // In focus mode only certain elements in the map can be selected.
    // It's also possible to select single parts of selection groups
    // without disbanding the group.
    virtual bool selectionFocusIsActive() = 0;

    // Returns the volume the focus items are occupying
    virtual AABB getSelectionFocusBounds() = 0;
};

}

constexpr const char* const MODULE_SELECTIONSYSTEM("SelectionSystem");

inline selection::SelectionSystem& GlobalSelectionSystem()
{
	static module::InstanceReference<selection::SelectionSystem> _reference(MODULE_SELECTIONSYSTEM);
	return _reference;
}

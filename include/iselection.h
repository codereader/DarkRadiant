/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_ISELECTION_H)
#define INCLUDED_ISELECTION_H

#include <cstddef>
#include "imodule.h"
#include "inode.h"
#include "generic/callbackfwd.h"
#include "signal/signalfwd.h"

class Renderer;
class View;

/**
 * greebo: A Selectable is everything that can be highlighted
 *         by the user in the scene (e.g. by interaction with the mouse).
 */
class Selectable
{
public:
	// Set the selection status of this object
	virtual void setSelected(bool select) = 0;

	// Check the selection status of this object (TRUE == selected)
	virtual bool isSelected() const = 0;

	// Toggle the selection status
	virtual void invertSelected() = 0;
};
typedef boost::shared_ptr<Selectable> SelectablePtr;

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
template<typename Element> class BasicVector4;
typedef BasicVector4<double> Vector4;
class Matrix4;
typedef Vector4 Quaternion;

typedef Callback1<const Selectable&> SelectionChangeCallback;
typedef SignalHandler1<const Selectable&> SelectionChangeHandler;

class SelectionInfo;

const std::string MODULE_SELECTIONSYSTEM("SelectionSystem");

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

  enum EMode
  {
    eEntity,
    ePrimitive,
    eComponent,
  };

	// The possible modes when in "component manipulation mode"
	enum EComponentMode {
		eDefault,	
		eVertex,
		eEdge,
		eFace,
	};

	// The possible manipulator modes
	enum EManipulatorMode {
		eTranslate,
		eRotate,
		eScale,
		eDrag,
		eClip,
	};

	/** greebo: An SelectionSystem::Observer gets notified
	 * as soon as the selection is changed.
	 */
	class Observer 
	{
	public:
		/** greebo: This gets called upon selection change.
		 * 	
		 * @instance: The instance that got affected (this may also be the parent brush of a FaceInstance).
		 * @isComponent: is TRUE if the changed selectable is a component (like a FaceInstance, VertexInstance).
		 */
		virtual void selectionChanged(const scene::INodePtr& node, bool isComponent) = 0;
	};
	
	virtual void addObserver(Observer* observer) = 0;
	virtual void removeObserver(Observer* observer) = 0;

	virtual const SelectionInfo& getSelectionInfo() = 0;

  virtual void SetMode(EMode mode) = 0;
  virtual EMode Mode() const = 0;
  virtual void SetComponentMode(EComponentMode mode) = 0;
  virtual EComponentMode ComponentMode() const = 0;
  virtual void SetManipulatorMode(EManipulatorMode mode) = 0;
  virtual EManipulatorMode ManipulatorMode() const = 0;

  virtual std::size_t countSelected() const = 0;
  virtual std::size_t countSelectedComponents() const = 0;
  virtual void onSelectedChanged(const scene::INodePtr& node, const Selectable& selectable) = 0;
  virtual void onComponentSelection(const scene::INodePtr& node, const Selectable& selectable) = 0;

	virtual scene::INodePtr ultimateSelected() const = 0;
	virtual scene::INodePtr penultimateSelected() const = 0;

  virtual void setSelectedAll(bool selected) = 0;
  virtual void setSelectedAllComponents(bool selected) = 0;

    /**
     * @brief
     * Visitor interface the for the selection system.
     *
     * This defines the Visitor interface which is used in the foreachSelected()
     * and foreachSelectedComponent() visit methods.
     */
	class Visitor
	{
	public:

        /**
         * @brief
         * Called by the selection system for each visited node.
         */
		virtual void visit(const scene::INodePtr& node) const = 0;
	};

    /**
     * @brief
     * Use the provided Visitor object to enumerate each selected node.
     */
    virtual void foreachSelected(const Visitor& visitor) const = 0;

    /**
     * @brief
     * Use the provided Visitor object to enumerate each selected component.
     */
    virtual void foreachSelectedComponent(const Visitor& visitor) const = 0;

  virtual void addSelectionChangeCallback(const SelectionChangeHandler& handler) = 0;

  virtual void NudgeManipulator(const Vector3& nudge, const Vector3& view) = 0;

  virtual void translateSelected(const Vector3& translation) = 0;
  virtual void rotateSelected(const Quaternion& rotation) = 0;
  virtual void scaleSelected(const Vector3& scaling) = 0;

  virtual void pivotChanged() const = 0;
  
  virtual bool SelectManipulator(const View& view, const double device_point[2], const double device_epsilon[2]) = 0;
  virtual void SelectPoint(const View& view, const double device_point[2], const double device_epsilon[2], EModifier modifier, bool face) = 0;
  virtual void SelectArea(const View& view, const double device_point[2], const double device_delta[2], EModifier modifier, bool face) = 0;
  
  virtual void MoveSelected(const View& view, const double device_point[2]) = 0;
  virtual void endMove() = 0;
  virtual void cancelMove() = 0;
};

inline SelectionSystem& GlobalSelectionSystem() {
	// Cache the reference locally
	static SelectionSystem& _selectionSystem(
		*boost::static_pointer_cast<SelectionSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_SELECTIONSYSTEM)
		)
	);
	return _selectionSystem;
}

#endif

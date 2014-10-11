#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

class wxTextCtrl;
class wxSpinCtrl;
class wxSpinCtrlDouble;

namespace objectives 
{

namespace ce
{

/**
 * ComponentEditor subclass for COMP_DISTANCE component type.
 *
 * An COMP_DISTANCE component usually uses a two specifiers.
 */
class DistanceComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_DISTANCE().getName(),
				ComponentEditorPtr(new DistanceComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// Name of the entity in question
	wxTextCtrl* _entity;

	// Name of the location entity
	wxTextCtrl* _location;

	// The distance between these two
	wxSpinCtrl* _distance;

	// The evaluation interval in seconds
	wxSpinCtrlDouble* _interval;

	/**
	 * Construct a default DistanceComponentEditor.
	 */
	DistanceComponentEditor() :
		_component(NULL),
		_entity(NULL),
		_location(NULL),
		_distance(NULL),
		_interval(NULL)
	{}

	/**
	 * Construct a DistanceComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	DistanceComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const {
		return ComponentEditorPtr(new DistanceComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

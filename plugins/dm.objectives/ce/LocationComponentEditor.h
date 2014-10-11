#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives
{

namespace ce
{

/**
 * ComponentEditor subclass for COMP_LOCATION component type.
 *
 * An COMP_LOCATION component usually uses a two specifiers.
 */
class LocationComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_LOCATION().getName(),
				ComponentEditorPtr(new LocationComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the entity in question
	SpecifierEditCombo* _entSpec;

	// SpecifierEditCombo for the location the entity should be in (or not)
	SpecifierEditCombo* _locationSpec;

	/**
	 * Construct a default LocationComponentEditor.
	 */
	LocationComponentEditor() :
		_component(NULL)
	{}

	/**
	 * Construct a LocationComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	LocationComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const 
	{
		return ComponentEditorPtr(new LocationComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

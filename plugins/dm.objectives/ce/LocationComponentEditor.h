#ifndef LOCATION_COMPONENT_EDITOR_H_
#define LOCATION_COMPONENT_EDITOR_H_

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

public:

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
	LocationComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new LocationComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* LOCATION_COMPONENT_EDITOR_H_ */

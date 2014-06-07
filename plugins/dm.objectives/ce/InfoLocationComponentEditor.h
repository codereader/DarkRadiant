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
 * ComponentEditor subclass for COMP_INFO_LOCATION component type.
 *
 * An COMP_INFO_LOCATION component usually uses a two specifiers.
 */
class InfoLocationComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_INFO_LOCATION().getName(),
				ComponentEditorPtr(new InfoLocationComponentEditor())
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
	 * Construct a default InfoLocationComponentEditor.
	 */
	InfoLocationComponentEditor() :
		_component(NULL)
	{}

	/**
	 * Construct a InfoLocationComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	InfoLocationComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const 
	{
		return ComponentEditorPtr(new InfoLocationComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

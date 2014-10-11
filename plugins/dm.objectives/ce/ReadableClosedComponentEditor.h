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
 * ComponentEditor subclass for COMP_READABLE_CLOSED component type.
 *
 * This component requires that the player closes a certain readable
 * (after having it opened before).
 */
class ReadableClosedComponentEditor :
	public ComponentEditorBase
{
private:
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_READABLE_CLOSED().getName(),
				ComponentEditorPtr(new ReadableClosedComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the readable
	SpecifierEditCombo* _readableSpec;

	/**
	 * Construct a default ReadableClosedComponentEditor.
	 */
	ReadableClosedComponentEditor() :
		_component(NULL)
	{}

	/**
	 * Construct an ReadableClosedComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	ReadableClosedComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const 
	{
		return ComponentEditorPtr(new ReadableClosedComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

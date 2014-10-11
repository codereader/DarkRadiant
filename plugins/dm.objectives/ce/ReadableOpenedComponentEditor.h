#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_READABLE_OPENED component type.
 *
 * This component requires that the player opens a certain readable.
 */
class ReadableOpenedComponentEditor :
	public ComponentEditorBase
{
private:
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_READABLE_OPENED().getName(),
				ComponentEditorPtr(new ReadableOpenedComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the readable
	SpecifierEditCombo* _readableSpec;

	/**
	 * Construct a default ReadableOpenedComponentEditor.
	 */
	ReadableOpenedComponentEditor() :
		_component(NULL)
	{}

	/**
	 * Construct an ReadableOpenedComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	ReadableOpenedComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new ReadableOpenedComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

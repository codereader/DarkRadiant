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
 * ComponentEditor subclass for COMP_CUSTOM_ASYNC component type.
 *
 * A COMP_CUSTOM_ASYNC component requires no specifiers, the state of these
 * components are controlled manually (by script or trigger).
 */
class CustomComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_CUSTOM_ASYNC().getName(),
				ComponentEditorPtr(new CustomComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	/**
	 * Construct a default CustomComponentEditor.
	 */
	CustomComponentEditor() :
		_component(NULL)
	{}

	/**
	 * Construct an CustomComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	CustomComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new CustomComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

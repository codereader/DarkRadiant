#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

class wxSpinCtrlDouble;
class wxTextCtrl;

namespace objectives 
{

namespace ce
{

/**
 * ComponentEditor subclass for COMP_CUSTOM_CLOCKED component type.
 *
 * An COMP_CUSTOM_CLOCKED component requires no specifiers, just one arguments
 * and a special spawnarg.
 */
class CustomClockedComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_CUSTOM_CLOCKED().getName(),
				ComponentEditorPtr(new CustomClockedComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// The text field for the script function name
	wxTextCtrl* _scriptFunction;

	// The spin button for the clock interval
	wxSpinCtrlDouble* _interval;

	/**
	 * Construct a default CustomClockedComponentEditor.
	 */
	CustomClockedComponentEditor() :
		_component(NULL),
		_scriptFunction(NULL),
		_interval(NULL)
	{}

	/**
	 * Construct an CustomClockedComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	CustomClockedComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new CustomClockedComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

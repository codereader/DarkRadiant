#ifndef CUSTOM_CLOCKED_COMPONENT_EDITOR_H_
#define CUSTOM_CLOCKED_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk
{
	class Entry;
	class SpinButton;
}

namespace objectives {

namespace ce {

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
	Gtk::Entry* _scriptFunction;

	// The spin button for the clock interval
	Gtk::SpinButton* _interval;

public:

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
	CustomClockedComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new CustomClockedComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* CUSTOM_CLOCKED_COMPONENT_EDITOR_H_ */

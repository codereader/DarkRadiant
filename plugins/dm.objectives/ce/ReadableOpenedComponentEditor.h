#ifndef _READABLE_OPENED_COMPONENT_EDITOR_H_
#define _READABLE_OPENED_COMPONENT_EDITOR_H_

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

public:

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
	ReadableOpenedComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new ReadableOpenedComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* _READABLE_OPENED_COMPONENT_EDITOR_H_ */

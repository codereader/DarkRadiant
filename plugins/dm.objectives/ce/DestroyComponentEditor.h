#ifndef DESTROY_COMPONENT_EDITOR_H_
#define DESTROY_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk { class SpinButton; }

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_DESTROY component type.
 *
 * An COMP_DESTROY component requires that the player destructs an inanimate item.
 */
class DestroyComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_DESTROY().getName(),
				ComponentEditorPtr(new DestroyComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the item
	SpecifierEditCombo* _itemSpec;

	// The spin button to specify the amount of AI to be killed
	Gtk::SpinButton* _amount;

public:

	/**
	 * Construct a default DestroyComponentEditor.
	 */
	DestroyComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{}

	/**
	 * Construct an DestroyComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	DestroyComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new DestroyComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* DESTROY_COMPONENT_EDITOR_H_ */

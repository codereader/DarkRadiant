#ifndef PICKPOCKET_COMPONENT_EDITOR_H_
#define PICKPOCKET_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk { class SpinButton; }

namespace objectives
{

namespace ce
{

/**
 * ComponentEditor subclass for COMP_PICKPOCKET component type.
 *
 * An COMP_PICKPOCKET component uses a single specifier and one argument.
 */
class PickpocketComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_PICKPOCKET().getName(),
				ComponentEditorPtr(new PickpocketComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the item
	SpecifierEditCombo* _itemSpec;

	// The spin button with the amount of pickpockets
	Gtk::SpinButton* _amount;

public:

	/**
	 * Construct a default PickpocketComponentEditor.
	 */
	PickpocketComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{}

	/**
	 * Construct a PickpocketComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	PickpocketComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new PickpocketComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* PICKPOCKET_COMPONENT_EDITOR_H_ */

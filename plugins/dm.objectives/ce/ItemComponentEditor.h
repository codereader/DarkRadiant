#ifndef _ITEM_COMPONENT_EDITOR_H_
#define _ITEM_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk { class SpinButton; }

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_ITEM component type.
 *
 * An ITEM component requires that the player acquires or loses an item.
 */
class ItemComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_ITEM().getName(),
				ComponentEditorPtr(new ItemComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the item
	SpecifierEditCombo* _itemSpec;

	// The spin button for the amount of items
	Gtk::SpinButton* _amount;

public:

	/**
	 * Construct a default ItemComponentEditor.
	 */
	ItemComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{}

	/**
	 * Construct an ItemComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	ItemComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new ItemComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* _ITEM_COMPONENT_EDITOR_H_ */

#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

class wxSpinCtrl;

namespace objectives 
{

namespace ce
{

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
	wxSpinCtrl* _amount;

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
	ItemComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new ItemComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

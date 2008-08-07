#ifndef _ITEM_COMPONENT_EDITOR_H_
#define _ITEM_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_ITEM component type.
 * 
 * An ITEM component requires that the player acquires or loses an item.
 */
class ItemComponentEditor : 
	public ComponentEditor
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
	
	// Main widget
	GtkWidget* _widget;
	
	// Component to edit
	Component* _component;	
	
	// SpecifierEditCombo for the item
	SpecifierEditCombo _itemSpec;
	
public:

	/**
	 * Construct a default ItemComponentEditor.
	 */
	ItemComponentEditor() : 
		_component(NULL)
	{}
	
	/**
	 * Construct an ItemComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	ItemComponentEditor(Component& component);
	
	/**
	 * Destructor
	 */
	~ItemComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new ItemComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* _ITEM_COMPONENT_EDITOR_H_ */

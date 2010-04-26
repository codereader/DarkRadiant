#ifndef _READABLE_PAGE_REACHED_COMPONENT_EDITOR_H_
#define _READABLE_PAGE_REACHED_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_READABLE_PAGE_REACHED component type.
 * 
 * This component requires that the player views a certain page of a readable.
 */
class ReadablePageReachedComponentEditor : 
	public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper()
		{
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_READABLE_PAGE_REACHED().getName(), 
				ComponentEditorPtr(new ReadablePageReachedComponentEditor())
			);
		}
	} regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
	// Component to edit
	Component* _component;	
	
	// SpecifierEditCombo for the item
	SpecifierEditCombo _readableSpec;

	// The spin button for the page number
	GtkWidget* _pageNum;
	
public:

	/**
	 * Construct a default ReadablePageReachedComponentEditor.
	 */
	ReadablePageReachedComponentEditor() : 
		_widget(NULL),
		_component(NULL),
		_pageNum(NULL)
	{}
	
	/**
	 * Construct an ReadablePageReachedComponentEditor with a 
	 * Component object to display and edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	ReadablePageReachedComponentEditor(Component& component);
	
	/**
	 * Destructor
	 */
	~ReadablePageReachedComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new ReadablePageReachedComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* _READABLE_PAGE_REACHED_COMPONENT_EDITOR_H_ */

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
 * ComponentEditor subclass for COMP_READABLE_PAGE_REACHED component type.
 *
 * This component requires that the player views a certain page of a readable.
 */
class ReadablePageReachedComponentEditor :
	public ComponentEditorBase
{
private:
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

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the item
	SpecifierEditCombo* _readableSpec;

	// The spin button for the page number
	wxSpinCtrl* _pageNum;

	/**
	 * Construct a default ReadablePageReachedComponentEditor.
	 */
	ReadablePageReachedComponentEditor() :
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
	ReadablePageReachedComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const 
	{
		return ComponentEditorPtr(new ReadablePageReachedComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

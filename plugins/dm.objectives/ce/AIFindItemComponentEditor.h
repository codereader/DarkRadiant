#pragma once

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_AI_FIND_ITEM component type.
 *
 * An COMP_AI_FIND_ITEM component doesn't use specifiers.
 */
class AIFindItemComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_AI_FIND_ITEM().getName(),
				ComponentEditorPtr(new AIFindItemComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	/**
	 * Construct a default AIFindItemComponentEditor.
	 */
	AIFindItemComponentEditor() :
		_component(NULL)
	{}

	/**
	 * Construct a AIFindItemComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	AIFindItemComponentEditor(wxWindow* parent, Component& component);

public:
	/* ComponentEditor implementation */

	ComponentEditorPtr create(wxWindow* parent, Component& component) const
	{
		return ComponentEditorPtr(new AIFindItemComponentEditor(parent, component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

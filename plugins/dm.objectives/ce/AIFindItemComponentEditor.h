#ifndef AI_FIND_ITEM_COMPONENT_EDITOR_H_
#define AI_FIND_ITEM_COMPONENT_EDITOR_H_

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

public:

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
	AIFindItemComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new AIFindItemComponentEditor(component));
	}

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* AI_FIND_ITEM_COMPONENT_EDITOR_H_ */

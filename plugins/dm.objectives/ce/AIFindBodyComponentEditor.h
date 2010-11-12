#ifndef AI_FIND_BODY_COMPONENT_EDITOR_H_
#define AI_FIND_BODY_COMPONENT_EDITOR_H_

#include "ComponentEditorBase.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace Gtk { class SpinButton; }

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_AI_FIND_BODY component type.
 *
 * An COMP_AI_FIND_BODY component doesn't use specifiers.
 */
class AIFindBodyComponentEditor :
	public ComponentEditorBase
{
	// Registration class
	static struct RegHelper
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_AI_FIND_BODY().getName(),
				ComponentEditorPtr(new AIFindBodyComponentEditor())
			);
		}
	} regHelper;

	// Component to edit
	Component* _component;

	// SpecifierEditCombo for the body to be found
	SpecifierEditCombo* _bodyCombo;

	// The spin button to specify the amount of AI to be knocked out
	Gtk::SpinButton* _amount;

public:

	/**
	 * Construct a default AIFindBodyComponentEditor.
	 */
	AIFindBodyComponentEditor() :
		_component(NULL),
		_amount(NULL)
	{}

	/**
	 * Construct a AIFindBodyComponentEditor with a Component object to display and
	 * edit.
	 *
	 * @param component
	 * The Component to edit.
	 */
	AIFindBodyComponentEditor(Component& component);

	/* ComponentEditor implementation */

	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new AIFindBodyComponentEditor(component));
	}

	void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* AI_FIND_BODY_COMPONENT_EDITOR_H_ */
